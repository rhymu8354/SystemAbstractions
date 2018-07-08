/**
 * @file StringExtensionsTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions functions which extend the string library.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <stdint.h>
#include <SystemAbstractions/StringExtensions.hpp>
#include <vector>

std::string vsprintfHelper(char const* format, ...) {
    va_list args;
    va_start(args, format);
    const auto result = SystemAbstractions::vsprintf("%s, %s!", args);
    va_end(args);
    return result;
}

TEST(StringExtensionsTests, vsprintf) {
    ASSERT_EQ("Hello, World!", vsprintfHelper("%s, %s!", "Hello", "World"));
}

TEST(StringExtensionsTests, sprintfBehavesLikeTheStandardCLibrarysVersion) {
    ASSERT_EQ("Hello, World!", SystemAbstractions::sprintf("%s, %s!", "Hello", "World"));
}

TEST(StringExtensionsTests, sprintfReturnsSomethingComparableToCppString) {
    const std::string expectedOutput = "The answer is 42.";
    ASSERT_EQ(expectedOutput, SystemAbstractions::sprintf("The answer is %d.", 42));
}

TEST(StringExtensionsTests, wcstombs) {
    ASSERT_EQ("Hello, World!", SystemAbstractions::wcstombs(L"Hello, World!"));
}
