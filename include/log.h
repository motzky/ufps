#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <source_location>
#include <string>

#include "config.h"
#include "utils/exception.h"
#include "utils/formatter.h"

namespace ufps::log
{
    namespace impl
    {
        inline static auto log_file = []
        {
            auto f = std::ofstream{"log", std::ios::app};
            if (!f)
            {
                throw Exception("could not open log file");
            }

            return f;
        }();
    }

    enum class Level
    {
        DEBUG,
        INFO,
        WARN,
#ifndef WIN32
        ERROR
#else
        // We cannot use ERROR, if we included <windows.h> since that has a define for ERROR which then leads to compile errors here
        ERR
#endif
    };

    template <Level L, class... Args>
    struct Print
    {
        Print(std::format_string<Args...> msg, Args &&...args, std::source_location loc = std::source_location::current())
        {
            auto level = "?";
            if constexpr (L == Level::DEBUG)
            {
                if constexpr (!config::log_enable_debug)
                {
                    return;
                }

                level = "DEBUG";
            }
            else if constexpr (L == Level::INFO)
            {
                level = "INFO ";
            }
            else if constexpr (L == Level::WARN)
            {
                level = "WARN ";
            }
#ifndef WIN32
            else if constexpr (L == Level::ERROR)
#else
            else if constexpr (L == Level::ERR)
#endif
            {
                level = "ERROR";
            }

            const auto path = std::filesystem::path{loc.file_name()};
            const auto log_line = std::format(
                "[{}] ({}:{}) - {}", level, path.filename().string(), loc.line(), std::format(msg, std::forward<Args>(args)...));
            std::println("{}", log_line);

            if constexpr (config::log_to_file)
            {
                impl::log_file << log_line << std::endl;
            }
        }
    };

    template <Level L = Level::INFO, class... Args>
    Print(std::format_string<Args...>, Args &&...) -> Print<L, Args...>;

    template <class... Args>
    using debug = Print<Level::DEBUG, Args...>;

    template <class... Args>
    using info = Print<Level::INFO, Args...>;

    template <class... Args>
    using warn = Print<Level::WARN, Args...>;

    template <class... Args>
#ifndef WIN32
    using error = Print<Level::ERROR, Args...>;
#else
    using error = Print<Level::ERR, Args...>;
#endif
}