/**
 * @file StringFileTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::StringFile class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <stdint.h>
#include <SystemAbstractions/StringFile.hpp>
#include <vector>

TEST(StringFileTests, WriteAndReadBack) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!\r\n";
    ASSERT_EQ(
        testString.length(),
        sf.Write(testString.data(), testString.length())
    );
    sf.SetPosition(0);
    SystemAbstractions::IFile::Buffer buffer(testString.length());
    ASSERT_EQ(
        testString.length(),
        sf.Read(buffer)
    );
    ASSERT_EQ(
        testString,
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
}

TEST(StringFileTests, ReadBackWithSizeAndOffsets) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!\r\n";
    (void)sf.Write(testString.data(), testString.length());
    sf.SetPosition(7);
    SystemAbstractions::IFile::Buffer buffer(9);
    ASSERT_EQ(
        5,
        sf.Read(buffer, 5, 3)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{0, 0, 0, 'W', 'o', 'r', 'l', 'd', 0}),
        buffer
    );
}

TEST(StringFileTests, ReadAdvancesFilePointer) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!";
    (void)sf.Write(testString.data(), testString.length());
    sf.SetPosition(0);
    SystemAbstractions::IFile::Buffer buffer(5);
    ASSERT_EQ(
        buffer.size(),
        sf.Read(buffer)
    );
    ASSERT_EQ(
        "Hello",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(5, sf.GetPosition());
    ASSERT_EQ(
        buffer.size(),
        sf.Read(buffer)
    );
    ASSERT_EQ(
        ", Wor",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(10, sf.GetPosition());
    ASSERT_EQ(
        3,
        sf.Read(buffer)
    );
    ASSERT_EQ(
        "ld!or",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(13, sf.GetPosition());
}

TEST(StringFileTests, PeakDoesNotAdvancesFilePointer) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!\r\n";
    (void)sf.Write(testString.data(), testString.length());
    sf.SetPosition(0);
    SystemAbstractions::IFile::Buffer buffer(5);
    ASSERT_EQ(
        buffer.size(),
        sf.Read(buffer)
    );
    ASSERT_EQ(
        "Hello",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(5, sf.GetPosition());
    ASSERT_EQ(
        4,
        sf.Peek(buffer, 4)
    );
    ASSERT_EQ(
        ", Woo",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(5, sf.GetPosition());
    ASSERT_EQ(
        buffer.size(),
        sf.Read(buffer)
    );
    ASSERT_EQ(
        ", Wor",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(10, sf.GetPosition());
}

TEST(StringFileTests, GetSize) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!\r\n";
    ASSERT_EQ(0, sf.GetSize());
    (void)sf.Write(testString.data(), testString.length());
    ASSERT_EQ(testString.length(), sf.GetSize());
}

TEST(StringFileTests, SetSize) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!\r\n";
    (void)sf.Write(testString.data(), testString.length());
    ASSERT_TRUE(sf.SetSize(5));
    ASSERT_EQ(5, sf.GetSize());
    SystemAbstractions::IFile::Buffer buffer(8);
    ASSERT_EQ(
        0,
        sf.Peek(buffer)
    );
    ASSERT_EQ(
        0,
        sf.Read(buffer)
    );
    sf.SetPosition(0);
    ASSERT_EQ(
        5,
        sf.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', 0, 0, 0}),
        buffer
    );
    ASSERT_TRUE(sf.SetSize(20));
    ASSERT_EQ(20, sf.GetSize());
    buffer.resize(20);
    sf.SetPosition(0);
    ASSERT_EQ(
        20,
        sf.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
        buffer
    );
}

TEST(StringFileTests, Clone) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!\r\n";
    (void)sf.Write(testString.data(), testString.length());
    sf.SetPosition(0);
    const auto clone = sf.Clone();
    sf.SetPosition(5);
    (void)sf.Write("FeelsBadMan", 11);
    SystemAbstractions::IFile::Buffer buffer(testString.length());
    ASSERT_EQ(0, clone->GetPosition());
    ASSERT_EQ(
        testString.length(),
        clone->Read(buffer)
    );
    ASSERT_EQ(
        testString,
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
}

TEST(StringFileTests, WriteBeyondEndAndIntoMiddle) {
    SystemAbstractions::StringFile sf;
    const std::string testString = "Hello, World!\r\n";
    (void)sf.Write(testString.data(), 5);
    sf.SetPosition(7);
    (void)sf.Write(testString.data() + 7, 8);
    ASSERT_EQ(testString.length(), sf.GetSize());
    SystemAbstractions::IFile::Buffer buffer(testString.length());
    sf.SetPosition(0);
    ASSERT_EQ(
        testString.length(),
        sf.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', 0, 0, 'W', 'o', 'r', 'l', 'd', '!', '\r', '\n'}),
        buffer
    );
    sf.SetPosition(5);
    (void)sf.Write(testString.data() + 5, 2);
    ASSERT_EQ(testString.length(), sf.GetSize());
    sf.SetPosition(0);
    ASSERT_EQ(
        testString.length(),
        sf.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\r', '\n'}),
        buffer
    );
}
