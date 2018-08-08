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

TEST(StringExtensionsTests, Trim) {
    ASSERT_EQ("Hello, World!", SystemAbstractions::Trim("  \t  \t\t  Hello, World! \r  \n \r\n \t \t\t  "));
}

TEST(StringExtensionsTests, Indent) {
    ASSERT_EQ(
        "Hello, World!\r\n"
        "  This is line 2\r\n"
        "  This is line 3\r\n",
        SystemAbstractions::Indent(
            "Hello, World!\r\n"
            "This is line 2\r\n"
            "This is line 3\r\n",
            2
        )
    );
    ASSERT_EQ(
        (
            "Struct {\r\n"
            "  field 1\r\n"
            "  field 2\r\n"
            "}"
        ),
        "Struct {"
        + SystemAbstractions::Indent(
            "\r\nfield 1"
            "\r\nfield 2",
            2
        )
        + "\r\n}"
    );
}

TEST(StringExtensionsTests, ParseComponent) {
    const std::string line = "Value = {abc {x} = def} NextValue = 42";
    ASSERT_EQ(
        "abc {x} = def}",
        SystemAbstractions::ParseComponent(line, 9, line.length())
    );
}

TEST(StringExtensionsTests, Escape) {
    const std::string line = "Hello, W^orld!";
    ASSERT_EQ(
        "Hello,^ W^^orld^!",
        SystemAbstractions::Escape(line, '^', {' ', '!', '^'})
    );
}

TEST(StringExtensionsTests, Unescape) {
    const std::string line = "Hello,^ W^^orld^!";
    ASSERT_EQ(
        "Hello, W^orld!",
        SystemAbstractions::Unescape(line, '^')
    );
}

TEST(StringExtensionsTests, Split) {
    const std::string line = "Hello, World!";
    ASSERT_EQ(
        (std::vector< std::string >{"Hello,", "World!"}),
        SystemAbstractions::Split(line, ' ')
    );
}

TEST(StringExtensionsTests, Join) {
    const std::vector< std::string > pieces{"Hello", "World!"};
    ASSERT_EQ(
        "Hello, World!",
        SystemAbstractions::Join(pieces, ", ")
    );
}

TEST(StringExtensionsTests, ToLower) {
    EXPECT_EQ("hello", SystemAbstractions::ToLower("Hello"));
    EXPECT_EQ("hello", SystemAbstractions::ToLower("hello"));
    EXPECT_EQ("hello", SystemAbstractions::ToLower("heLLo"));
    EXPECT_EQ("example", SystemAbstractions::ToLower("eXAmplE"));
    EXPECT_EQ("example", SystemAbstractions::ToLower("example"));
    EXPECT_EQ("example", SystemAbstractions::ToLower("EXAMPLE"));
    EXPECT_EQ("foo1bar", SystemAbstractions::ToLower("foo1BAR"));
    EXPECT_EQ("foo1bar", SystemAbstractions::ToLower("fOo1bAr"));
    EXPECT_EQ("foo1bar", SystemAbstractions::ToLower("foo1bar"));
    EXPECT_EQ("foo1bar", SystemAbstractions::ToLower("FOO1BAR"));
}
