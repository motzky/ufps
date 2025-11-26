#pragma once

#include <exception>
#include <format>
#include <memory>
#include <string_view>
#include <utility>

#include "log.h"
#include "utils/auto_release.h"
#include "utils/exception.h"
#include "utils/formatter.h"

namespace ufps
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
    template <class... Args>
    auto expect(bool predicate, std::format_string<Args...> msg, Args &&...args) -> void
    {
        if (!predicate)
        {
            log::error("{}", std::format(msg, std::forward<Args>(args)...));
            log::error("{}", std::stacktrace::current(2));
            std::terminate();
            std::unreachable();
        }
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    template <class... Args>
    auto ensure(bool predicate, std::format_string<Args...> msg, Args &&...args) -> void
    {
        if (!predicate)
        {
            throw Exception(msg, std::forward<Args>(args)...);
        }
    }

    template <class T, T Invalid, class... Args>
    auto ensure(AutoRelease<T, Invalid> &obj, std::format_string<Args...> msg, Args &&...args) -> void
    {
        ensure(!!obj, msg, std::forward<Args>(args)...);
    }

    template <class T, class D, class... Args>
    auto ensure(std::unique_ptr<T, D> &obj, std::format_string<Args...> msg, Args &&...args) -> void
    {
        ensure(obj != nullptr, msg, std::forward<Args>(args)...);
    }
}
