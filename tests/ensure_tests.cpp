#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "utils/ensure.h"
#include "utils/exception.h"

using namespace std::literals;

TEST(exception, simple)
{
    const auto ex = ufps::Exception("hello");

    ASSERT_EQ(ex.what(), "hello"sv);
}

TEST(ensure, expect_death)
{
    ASSERT_DEATH({ ufps::expect(false, "something bad"); }, "");
}

TEST(ensure, ensure_fail)
{
    ASSERT_THROW({ ufps::ensure(false, "something bad happened"); }, ufps::Exception);
}
