#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "utils/ensure.h"
#include "utils/exception.h"

using namespace std::literals;

TEST(ensure, expect_true)
{
    EXPECT_NO_THROW(ufps::expect(true, "This should not throw"));
}

TEST(ensure, ensure_true)
{
    EXPECT_NO_THROW(ufps::ensure(true, "This should not throw"));
}

TEST(ensure, expect_death)
{
    ASSERT_DEATH({ ufps::expect(false, "something bad"); }, "");
}

TEST(ensure, ensure_false)
{
    ASSERT_THROW({ ufps::ensure(false, "something bad happened"); }, ufps::Exception);
}

TEST(ensure, ensure_auto_release)
{
    ufps::AutoRelease<int *, nullptr> ptr{new int{}, [](int *p)
                                          { delete p; }};

    EXPECT_NO_THROW(ufps::ensure(ptr, "This should not throw"));

    ufps::AutoRelease<int *, nullptr> invalid_ptr{nullptr, nullptr};

    EXPECT_THROW(ufps::ensure(invalid_ptr, "This should throw"), ufps::Exception);
}

TEST(ensure, ensure_unique_ptr)
{
    std::unique_ptr<int> ptr = std::make_unique<int>(42);

    EXPECT_NO_THROW(ufps::ensure(ptr, "This should not throw"));

    std::unique_ptr<int> invalid_ptr;

    EXPECT_THROW(ufps::ensure(invalid_ptr, "This should throw"), ufps::Exception);
}
