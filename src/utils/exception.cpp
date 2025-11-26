#include "utils/exception.h"

#include <format>
#include <iostream>

namespace ufps
{
    auto Exception::stacktrace() const -> std::string
    {
        return std::to_string(_trace);
    }

    auto Exception::what() const -> std::string_view
    {
        return _what;
    }

    auto Exception::to_string() const -> std::string
    {
        return std::format("{}\n{}", what(), stacktrace());
    }

}
