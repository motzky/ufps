#include <cstddef>
#include <string_view>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "graphics/multi_buffer.h"
#include "utils/data_buffer.h"

using namespace std::literals;

struct FakeBuffer
{
    FakeBuffer(std::size_t size, std::string_view name)
        : size{size}, name{name}
    {
    }

    auto write(ufps::DataBufferView data, std::size_t offset) -> void
    {
        write_calls.push_back({data.data(), offset});
    }

    std::vector<std::tuple<const std::byte *, std::size_t>> write_calls;
    std::size_t size;
    std::string_view name;
};

TEST(multi_buffer, simple)
{
    auto data = ufps::DataBuffer{std::byte{0x0}, std::byte{0x1}, std::byte{0x2}};
    auto data_view = ufps::DataBufferView{data};

    auto mb = ufps::MultiBuffer<FakeBuffer, 3zu>{data_view.size_bytes(), "testbuffer"sv};
    mb.write(data_view, 0zu);

    const auto &buffer = mb.buffer();

    const auto expected = std::vector{std::make_tuple(data_view.data(), 0zu)};

    ASSERT_EQ(buffer.size, data_view.size_bytes() * 3zu);
    ASSERT_EQ(buffer.write_calls, expected);
}

TEST(multi_buffer, triple_write)
{
    auto data = ufps::DataBuffer{std::byte{0x0}, std::byte{0x1}, std::byte{0x2}};
    auto data_view = ufps::DataBufferView{data};

    auto mb = ufps::MultiBuffer<FakeBuffer, 3zu>{data_view.size_bytes(), "testbuffer"sv};
    mb.write(data_view, 0zu);
    mb.advance();
    mb.write(data_view, 0zu);
    mb.advance();
    mb.write(data_view, 0zu);
    mb.advance();

    const auto &buffer = mb.buffer();

    const auto expected = std::vector{
        std::make_tuple(data_view.data(), data_view.size_bytes() * 0zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 1zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 2zu),
    };

    ASSERT_EQ(buffer.size, data_view.size_bytes() * 3zu);
    ASSERT_EQ(buffer.write_calls, expected);
}

TEST(multi_buffer, quadruple_write)
{
    auto data = ufps::DataBuffer{std::byte{0x0}, std::byte{0x1}, std::byte{0x2}};
    auto data_view = ufps::DataBufferView{data};

    auto mb = ufps::MultiBuffer<FakeBuffer, 3zu>{data_view.size_bytes(), "testbuffer"sv};
    mb.write(data_view, 0zu);
    mb.advance();
    mb.write(data_view, 0zu);
    mb.advance();
    mb.write(data_view, 0zu);
    mb.advance();
    mb.write(data_view, 0zu);
    mb.advance();

    const auto &buffer = mb.buffer();

    const auto expected = std::vector{
        std::make_tuple(data_view.data(), data_view.size_bytes() * 0zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 1zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 2zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 0zu),
    };

    ASSERT_EQ(buffer.size, data_view.size_bytes() * 3zu);
    ASSERT_EQ(buffer.write_calls, expected);
}
TEST(multi_buffer, multi_write_offset)
{
    auto data = ufps::DataBuffer{std::byte{0x0}, std::byte{0x1}, std::byte{0x2}, std::byte{03}, std::byte{0x4}};
    auto data_view = ufps::DataBufferView{data};

    auto mb = ufps::MultiBuffer<FakeBuffer, 3zu>{data_view.size_bytes(), "testbuffer"sv};
    mb.write(data_view, 1zu);
    mb.advance();
    mb.write(data_view, 2zu);
    mb.advance();
    mb.write(data_view, 3zu);
    mb.advance();
    mb.write(data_view, 4zu);
    mb.advance();

    const auto &buffer = mb.buffer();

    const auto expected = std::vector{
        std::make_tuple(data_view.data(), data_view.size_bytes() * 0zu + 1zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 1zu + 2zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 2zu + 3zu),
        std::make_tuple(data_view.data(), data_view.size_bytes() * 0zu + 4zu),
    };

    ASSERT_EQ(buffer.size, data_view.size_bytes() * 3zu);
    ASSERT_EQ(buffer.write_calls, expected);
}
