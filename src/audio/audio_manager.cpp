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
#include "utils/ensure.h"
#include "utils/string_unordered_map.h"

namespace
{
    auto contains_cc(ufps::DataBufferView source, ufps::DataBufferView control_code) -> bool
    {
        const auto chunk_header = std::ranges::search(source, control_code);
        return !std::ranges::empty(chunk_header);
    }

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

namespace ufps
{
    AudioManager::AudioManager(ResourceLoader &resource_loader)
        : _tracks{},
          _xaudio{},
          _mastering_voice{}
    {
        ensure(::FAudioCreate(std::out_ptr(_xaudio), 0, FAUDIO_DEFAULT_PROCESSOR) == FAUDIO_OK, "failed to create FAudio");

        ensure(::FAudio_CreateMasteringVoice(_xaudio.get(), std::out_ptr(_mastering_voice),
                                             FAUDIO_DEFAULT_CHANNELS,
                                             FAUDIO_DEFAULT_SAMPLERATE,
                                             0u,
                                             0u,
                                             nullptr) == FAUDIO_OK,
               "failed to create mastering voice");

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

            const std::byte wave_cc[]{std::byte{'W'}, std::byte{'A'}, std::byte{'V'}, std::byte{'E'}};
            if (!contains_cc(track_data, wave_cc))
            {
                log::error("failed to parse wav: {} did not contain WAVE marker", resource);
                continue;
            }

            const std::byte fmt_cc[]{std::byte{'f'}, std::byte{'m'}, std::byte{'t'}, std::byte{' '}};
            const auto fmt_chunk = parse_chunk(track_data, fmt_cc);

            if (!fmt_chunk)
            {
                log::error("failed to parse wav: {} {}", resource, fmt_chunk.error());
                continue;
            }

            const std::byte data_cc[]{std::byte{'d'}, std::byte{'a'}, std::byte{'t'}, std::byte{'a'}};
            const auto data_chunk = parse_chunk(track_data, data_cc);

            if (!data_chunk)
            {
                log::error("failed to parse wav: {} {}", resource, data_chunk.error());
                continue;
            }

            const auto name = resource.substr(resource.find_last_of('/') + 1zu);

            const auto track = Track{
                .format = {std::ranges::cbegin(*fmt_chunk), std::ranges::cend(*fmt_chunk)},
                .data = {std::ranges::cbegin(*data_chunk), std::ranges::cend(*data_chunk)},
            };

            // The WAVE `fmt` chunk is a WAVEFORMATEX, whose layout matches the first
            // 16 bytes of FAudioWaveFormatEx. A canonical PCM chunk is only 16 bytes --
            // it omits the trailing cbSize field that makes FAudioWaveFormatEx 18 bytes
            // -- so copy whichever is smaller into a zero-initialized struct. That
            // populates every field FAudio's PCM path needs (format tag, channels,
            // sample rate, block align, bits per sample) while leaving cbSize at 0.
            // Passing each track's real format fixes the mis-decoded audio; FAudio then
            // resamples every source voice up to the mastering rate on its own.
            auto format = ::FAudioWaveFormatEx{};
            const auto copy_bytes = std::min<std::size_t>((*fmt_chunk).size(), sizeof(format));
            std::memcpy(&format, track.format.data(), copy_bytes);

            auto loaded = LoadedTrack{track, {}};
            ensure(::FAudio_CreateSourceVoice(_xaudio.get(), std::out_ptr(loaded.voice), &format, 0u, FAUDIO_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr) == FAUDIO_OK, "failed to create source voice");

            _tracks.emplace(std::move(name), std::move(loaded));
        }
    }

    auto AudioManager::play(std::string_view track_name) -> void
    {
        const auto track = _tracks.find(track_name);
        if (track == std::ranges::cend(_tracks))
        {
            log::warn("tried to play missing track: {}", track_name);
            return;
        }

        const auto &voice = track->second.voice;

        // Restart from the beginning on re-trigger: Stop() flushes any previously
        // queued buffer, so a rapid re-fire (e.g. firing the laser repeatedly) plays
        // the full clip again rather than piling buffers onto one another.
        ::FAudioSourceVoice_Stop(voice.get(), 0u, FAUDIO_COMMIT_NOW);

        const auto buffer = ::FAudioBuffer{
            .Flags = FAUDIO_END_OF_STREAM,
            .AudioBytes = static_cast<std::uint32_t>(std::ranges::size(track->second.track.data)),
            .pAudioData = reinterpret_cast<const std::uint8_t *>(std::ranges::data(track->second.track.data)),
            .PlayBegin = 0,
            .PlayLength = 0,
            .LoopBegin = 0,
            .LoopLength = 0,
            .LoopCount = 0,
            .pContext = nullptr,
        };

        ensure(FAudioSourceVoice_SubmitSourceBuffer(voice.get(), &buffer, nullptr) == FAUDIO_OK, "failed to set source");
        ensure(FAudioSourceVoice_Start(voice.get(), 0u, FAUDIO_COMMIT_NOW) == FAUDIO_OK, "failed to start sound");
    }
}