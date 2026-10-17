#include "audio/audio_manager.h"

#include <algorithm>
#include <cstring>
#include <expected>
#include <ranges>
#include <vector>

#include "audio/track.h"
#include "log.h"
#include "resources/resource_loader.h"
#include "utils/data_buffer.h"
#include "utils/string_unordered_map.h"

namespace
{
    auto parse_chunk(ufps::DataBufferView source, ufps::DataBufferView control_code) -> std::expected<ufps::DataBufferView, std::string>
    {
        const auto chunk_header = std::ranges::search(source, control_code);
        if (std::ranges::empty(chunk_header))
        {
            return std::unexpected("could not find control code in chunk");
        }

        const auto header_offset = std::ranges::distance(std::ranges::cbegin(source), std::ranges::cbegin(chunk_header));

        auto chunk_size = std::uint32_t{};
        std::memcpy(&chunk_size, source.data() + header_offset + chunk_header.size(), sizeof(chunk_size));

        if (static_cast<std::size_t>(header_offset) + chunk_header.size() + sizeof(chunk_size) + chunk_size > source.size())
        {
            return std::unexpected("chunk size exceeds buffer");
        }

        ufps::log::debug("header_offset: {:x}", header_offset);
        ufps::log::debug("chunk_size: {:x}", chunk_size);

        return std::span{source.data() + header_offset + chunk_header.size() + sizeof(chunk_size), chunk_size};
    }
}

namespace
{
    struct WavFormat
    {
        int sample_rate{};
        int channels{};
        int bit_depth{};
    };

    auto decode_fmt_chunk(ufps::DataBufferView fmt) -> std::expected<WavFormat, std::string>
    {
        if (fmt.size() < 16)
        {
            return std::unexpected("fmt chunk smaller than 16 bytes");
        }

        // The fmt chunk is little-endian; both target platforms are little-endian.
        // Layout: audio_format(2) channels(2) sample_rate(4) byte_rate(4) block_align(2) bit_depth(2)
        std::uint16_t audio_format{};
        std::uint16_t channels{};
        std::uint32_t sample_rate{};
        std::uint16_t bit_depth{};

        std::memcpy(&audio_format, fmt.data(), sizeof(audio_format));
        std::memcpy(&channels, fmt.data() + 2, sizeof(channels));
        std::memcpy(&sample_rate, fmt.data() + 4, sizeof(sample_rate));
        // fmt.data() + 8: byte_rate (4 bytes) - not needed
        // fmt.data() + 12: block_align (2 bytes) - not needed
        std::memcpy(&bit_depth, fmt.data() + 14, sizeof(bit_depth));

        if (audio_format != 1)
        {
            return std::unexpected("only PCM (audio format 1) is supported");
        }

        if (channels < 1 || channels > 2)
        {
            return std::unexpected("only mono and stereo are supported");
        }

        if (bit_depth != 8 && bit_depth != 16)
        {
            return std::unexpected("only 8-bit and 16-bit PCM are supported");
        }

        return WavFormat{.sample_rate = static_cast<int>(sample_rate), .channels = static_cast<int>(channels), .bit_depth = static_cast<int>(bit_depth)};
    }
}

namespace ufps
{
    AudioManager::AudioManager(ResourceLoader &resource_loader)
        : _tracks{}, _backend{create_audio_backend()}
    {
        for (const auto &resource : resource_loader.resources("sounds") |
                                        std::views::filter([](const auto &e)
                                                           { return e.ends_with(".wav"); }))
        {
            log::info("loading sound: {}", resource);
            const auto track_data = resource_loader.load_data_buffer(resource);

            const std::byte riff_cc[]{std::byte{'R'}, std::byte{'I'}, std::byte{'F'}, std::byte{'F'}};
            const auto riff_chunk = parse_chunk(track_data, riff_cc);

            if (!riff_chunk)
            {
                log::error("failed to parse wav: {} {}", resource, riff_chunk.error());
                continue;
            }

            // "WAVE" is the RIFF form id (4 bytes), not a chunk with a size field.
            // fmt/data chunks follow directly inside the RIFF chunk.
            const std::byte wave_id[]{std::byte{'W'}, std::byte{'A'}, std::byte{'V'}, std::byte{'E'}};
            if (!std::equal(wave_id, wave_id + 4, riff_chunk->data()))
            {
                log::error("failed to parse wav: {} is not a WAVE file", resource);
                continue;
            }

            const std::byte fmt_cc[]{std::byte{'f'}, std::byte{'m'}, std::byte{'t'}, std::byte{' '}};
            const auto fmt_chunk = parse_chunk(*riff_chunk, fmt_cc);

            if (!fmt_chunk)
            {
                log::error("failed to parse wav: {} {}", resource, fmt_chunk.error());
                continue;
            }

            const std::byte data_cc[]{std::byte{'d'}, std::byte{'a'}, std::byte{'t'}, std::byte{'a'}};
            const auto data_chunk = parse_chunk(*riff_chunk, data_cc);

            if (!data_chunk)
            {
                log::error("failed to parse wav: {} {}", resource, data_chunk.error());
                continue;
            }

            const auto format = decode_fmt_chunk(*fmt_chunk);

            if (!format)
            {
                log::error("unsupported wav format for {}: {}", resource, format.error());
                continue;
            }

            Track track{
                .format = {std::ranges::cbegin(*fmt_chunk), std::ranges::cend(*fmt_chunk)},
                .data = {std::ranges::cbegin(*data_chunk), std::ranges::cend(*data_chunk)},
                .sample_rate = format->sample_rate,
                .channels = format->channels,
                .bit_depth = format->bit_depth,
            };

            if (_backend)
            {
                track.buffer_handle = _backend->create_buffer(*data_chunk, track.sample_rate, track.channels, track.bit_depth);
                track.source_handle = _backend->create_source(track.buffer_handle);
            }

            log::debug("loaded sound {} ({} hz, {} channel(s), {} bit)", resource, track.sample_rate, track.channels, track.bit_depth);
            _tracks[resource.substr(resource.find_last_of('/'))] = std::move(track);
        }
    }

    AudioManager::~AudioManager()
    {
        if (!_backend)
        {
            return;
        }

        for (auto &track_entry : _tracks)
        {
            auto &track = track_entry.second;
            _backend->stop_source(track.source_handle);
            _backend->delete_source(track.source_handle);
            _backend->delete_buffer(track.buffer_handle);
        }
    }

    auto AudioManager::play(std::string_view track_name) -> void
    {
        const auto it = _tracks.find(track_name);
        if (it == _tracks.end())
        {
            log::warn("audio: no track named '{}'", track_name);
            return;
        }

        if (!_backend)
        {
            log::warn("audio: no backend available, cannot play '{}'", track_name);
            return;
        }

        _backend->play_source(it->second.source_handle);
    }

    auto AudioManager::stop(std::string_view track_name) -> void
    {
        const auto it = _tracks.find(track_name);
        if (it == _tracks.end())
        {
            log::warn("audio: no track named '{}'", track_name);
            return;
        }

        if (!_backend)
        {
            return;
        }

        _backend->stop_source(it->second.source_handle);
    }
}