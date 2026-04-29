#pragma once

#include <cstdint>
#include <format>
#include <stacktrace>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ufps
{
    class Exception
    {
    public:
        template <class... Args>
        constexpr Exception(std::format_string<Args...> what, Args &&...args)
            : _trace(std::stacktrace::current(1)),
              _what(std::format(what, std::forward<Args>(args)...))
        {
        }

        template <class... Args>
        constexpr Exception(std::format_string<Args...> what, Args &&...args, std::uint32_t skip)
            : _trace(std::stacktrace::current(skip)),
              _what(std::format(what, std::forward<Args>(args)...))
        {
        }

        constexpr auto stacktrace() const -> std::string;
        constexpr auto to_string() const -> std::string;
        constexpr auto what() const -> std::string_view;

    private:
        std::stacktrace _trace;
        std::string _what;
    };

    constexpr auto Exception::stacktrace() const -> std::string
    {
        return std::to_string(_trace);
    }

    constexpr auto Exception::what() const -> std::string_view
    {
        return _what;
    }

    constexpr auto Exception::to_string() const -> std::string
    {
        return std::format("{}\n{}", what(), stacktrace());
    }

}
