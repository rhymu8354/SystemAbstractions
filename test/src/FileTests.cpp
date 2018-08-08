/**
 * @file FileTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::File class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <set>
#include <SystemAbstractions/File.hpp>
#include <thread>

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct FileTests
    : public ::testing::Test
{
    // Properties

    /**
     * This is the temporary directory to use to test
     * the File class.
     */
    std::string testAreaPath;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
        testAreaPath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(testAreaPath));
    }

    virtual void TearDown() {
        bool failedToDeleteTestArea = true;
        for (size_t i = 0; i < 10; ++i) {
            if (SystemAbstractions::File::DeleteDirectory(testAreaPath)) {
                failedToDeleteTestArea = false;
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        ASSERT_FALSE(failedToDeleteTestArea);
    }
};

TEST_F(FileTests, BasicFileMethods) {
    // Check initial conditions:
    // - test area exists
    // - test file doe snot exist
    SystemAbstractions::File testArea(testAreaPath);
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_FALSE(file.IsExisting());
    ASSERT_FALSE(file.IsDirectory());
    ASSERT_FALSE(file.Open());
    ASSERT_TRUE(testArea.IsExisting());
    ASSERT_TRUE(testArea.IsDirectory());

    // Create file and verify it exists.
    ASSERT_TRUE(file.Create());
    ASSERT_TRUE(file.IsExisting());
    ASSERT_FALSE(file.IsDirectory());
    file.Close();

    // We should be able to open the file now that it exists.
    ASSERT_TRUE(file.Open());
    file.Close();

    // Now destroy the file and verify it not longer exists.
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());

    // Check that we can move the file while it's open.
    ASSERT_TRUE(file.Create());
    ASSERT_TRUE(file.IsExisting());
    ASSERT_EQ(testFilePath, file.GetPath());
    {
        SystemAbstractions::File fileCheck(testFilePath);
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    {
        SystemAbstractions::File fileCheck(testFilePath + "2");
        ASSERT_FALSE(fileCheck.IsExisting());
    }
    ASSERT_TRUE(file.Move(testFilePath + "2"));
    ASSERT_TRUE(file.IsExisting());
    ASSERT_NE(testFilePath, file.GetPath());
    ASSERT_EQ(testFilePath + "2", file.GetPath());
    {
        SystemAbstractions::File fileCheck(testFilePath);
        ASSERT_FALSE(fileCheck.IsExisting());
    }
    {
        SystemAbstractions::File fileCheck(testFilePath + "2");
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    file.Close();
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());

    // Check that we can move the file while it's not open.
    file = std::move(SystemAbstractions::File(testFilePath));
    ASSERT_TRUE(file.Create());
    file.Close();
    ASSERT_TRUE(file.IsExisting());
    ASSERT_EQ(testFilePath, file.GetPath());
    {
        SystemAbstractions::File fileCheck(testFilePath);
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    {
        SystemAbstractions::File fileCheck(testFilePath + "2");
        ASSERT_FALSE(fileCheck.IsExisting());
    }
    ASSERT_TRUE(file.Move(testFilePath + "2"));
    ASSERT_TRUE(file.IsExisting());
    ASSERT_NE(testFilePath, file.GetPath());
    ASSERT_EQ(testFilePath + "2", file.GetPath());
    {
        SystemAbstractions::File fileCheck(testFilePath);
        ASSERT_FALSE(fileCheck.IsExisting());
    }
    {
        SystemAbstractions::File fileCheck(testFilePath + "2");
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());

    // Check that we can copy a file.
    file = std::move(SystemAbstractions::File(testFilePath));
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World\r\n";
    ASSERT_EQ(testString.length(), file.Write(testString.data(), testString.length()));
    {
        SystemAbstractions::File file2(testFilePath + "2");
        ASSERT_FALSE(file2.IsExisting());
        ASSERT_TRUE(file.Copy(file2.GetPath()));
        ASSERT_TRUE(file2.IsExisting());
        ASSERT_TRUE(file2.Open());
        SystemAbstractions::IFile::Buffer buffer(file2.GetSize());
        ASSERT_EQ(buffer.size(), file2.Read(buffer));
        ASSERT_EQ(testString, std::string(buffer.begin(), buffer.end()));
        file2.Destroy();
        ASSERT_FALSE(file2.IsExisting());
    }
    file.Close();
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());
}

TEST_F(FileTests, DirectoryTests) {
    // Set up some test files in the test area.
    SystemAbstractions::File testArea(testAreaPath);
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World\r\n";
    ASSERT_EQ(testString.length(), file.Write(testString.data(), testString.length()));
    SystemAbstractions::File file2(testFilePath + "2");
    ASSERT_TRUE(file.Copy(file2.GetPath()));
    file.Close();

    // Create a subdirectory with one file in it.
    const std::string subPath = testAreaPath + "/sub";
    SystemAbstractions::File::CreateDirectory(subPath);
    SystemAbstractions::File sub(subPath);
    ASSERT_TRUE(sub.IsDirectory());
    const std::string testSubFilePath = subPath + "/bar.txt";
    SystemAbstractions::File file3(testSubFilePath);
    ASSERT_TRUE(file3.Create());
    const std::string testString2 = "Something else!\r\n";
    ASSERT_EQ(testString2.length(), file3.Write(testString2.data(), testString2.length()));
    file3.Close();

    // Get a list of the files.
    std::vector< std::string > list;
    SystemAbstractions::File::ListDirectory(testAreaPath, list);
    std::set< std::string > set(list.begin(), list.end());
    for (const std::string& expectedElement: { "foo.txt", "foo.txt2", "sub" }) {
        const auto element = set.find(testAreaPath + "/" + expectedElement);
        ASSERT_FALSE(element == set.end()) << expectedElement;
        (void)set.erase(element);
    }
    ASSERT_TRUE(set.empty());

    // Copy subdirectory and verify its one file inside got copied.
    const std::string subPath2 = testAreaPath + "/sub2";
    ASSERT_TRUE(SystemAbstractions::File::CopyDirectory(subPath, subPath2));
    SystemAbstractions::File sub2(subPath2);
    ASSERT_TRUE(sub2.IsDirectory());
    ASSERT_TRUE(sub2.IsExisting());
    const std::string testSubFilePath2 = subPath2 + "/bar.txt";
    SystemAbstractions::File file4(testSubFilePath2);
    ASSERT_TRUE(file4.Open());
    SystemAbstractions::IFile::Buffer buffer(file4.GetSize());
    ASSERT_EQ(buffer.size(), file4.Read(buffer));
    ASSERT_EQ(testString2, std::string(buffer.begin(), buffer.end()));
    file4.Close();

    // Destroy copy of subdirectory.
    ASSERT_TRUE(SystemAbstractions::File::DeleteDirectory(subPath2));
    ASSERT_FALSE(sub2.IsExisting());
    ASSERT_FALSE(file4.IsExisting());
}

TEST_F(FileTests, RepurposeFileObject) {
    const std::string testFilePath1 = testAreaPath + "/foo.txt";
    const std::string testFilePath2 = testAreaPath + "/bar.txt";
    SystemAbstractions::File file(testFilePath1);

    // What this does is implicitly cast testFilePath2, a std::string,
    // to a SystemAbtractions::File (since the latter class has
    // a non-explicit constructor with a single std::string argument),
    // and then move-assign it to the file instance.
    file = testFilePath2;
    ASSERT_EQ(testFilePath2, file.GetPath());
}

TEST_F(FileTests, WriteAndReadBack) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!\r\n";
    ASSERT_EQ(
        testString.length(),
        file.Write(testString.data(), testString.length())
    );
    file.SetPosition(0);
    SystemAbstractions::IFile::Buffer buffer(testString.length());
    ASSERT_EQ(
        testString.length(),
        file.Read(buffer)
    );
    ASSERT_EQ(
        testString,
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
}

TEST_F(FileTests, ReadBackWithSizeAndOffsets) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!\r\n";
    (void)file.Write(testString.data(), testString.length());
    file.SetPosition(7);
    SystemAbstractions::IFile::Buffer buffer(9);
    ASSERT_EQ(
        5,
        file.Read(buffer, 5, 3)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{0, 0, 0, 'W', 'o', 'r', 'l', 'd', 0}),
        buffer
    );
}

TEST_F(FileTests, ReadAdvancesFilePointer) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!";
    (void)file.Write(testString.data(), testString.length());
    file.SetPosition(0);
    SystemAbstractions::IFile::Buffer buffer(5);
    ASSERT_EQ(
        buffer.size(),
        file.Read(buffer)
    );
    ASSERT_EQ(
        "Hello",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(5, file.GetPosition());
    ASSERT_EQ(
        buffer.size(),
        file.Read(buffer)
    );
    ASSERT_EQ(
        ", Wor",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(10, file.GetPosition());
    ASSERT_EQ(
        3,
        file.Read(buffer)
    );
    ASSERT_EQ(
        "ld!or",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(13, file.GetPosition());
}

TEST_F(FileTests, PeakDoesNotAdvancesFilePointer) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!\r\n";
    (void)file.Write(testString.data(), testString.length());
    file.SetPosition(0);
    SystemAbstractions::IFile::Buffer buffer(5);
    ASSERT_EQ(
        buffer.size(),
        file.Read(buffer)
    );
    ASSERT_EQ(
        "Hello",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(5, file.GetPosition());
    ASSERT_EQ(
        4,
        file.Peek(buffer, 4)
    );
    ASSERT_EQ(
        ", Woo",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(5, file.GetPosition());
    ASSERT_EQ(
        buffer.size(),
        file.Read(buffer)
    );
    ASSERT_EQ(
        ", Wor",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
    ASSERT_EQ(10, file.GetPosition());
}

TEST_F(FileTests, GetSize) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!\r\n";
    ASSERT_EQ(0, file.GetSize());
    (void)file.Write(testString.data(), testString.length());
    ASSERT_EQ(testString.length(), file.GetSize());
}

TEST_F(FileTests, SetSize) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!\r\n";
    (void)file.Write(testString.data(), testString.length());
    ASSERT_TRUE(file.SetSize(5));
    ASSERT_EQ(5, file.GetSize());
    SystemAbstractions::IFile::Buffer buffer(8);
    ASSERT_EQ(
        0,
        file.Peek(buffer)
    );
    ASSERT_EQ(
        0,
        file.Read(buffer)
    );
    file.SetPosition(0);
    ASSERT_EQ(
        5,
        file.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', 0, 0, 0}),
        buffer
    );
    ASSERT_TRUE(file.SetSize(20));
    ASSERT_EQ(20, file.GetSize());
    buffer.resize(20);
    file.SetPosition(0);
    ASSERT_EQ(
        20,
        file.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}),
        buffer
    );
}

TEST_F(FileTests, Clone) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!\r\n";
    (void)file.Write(testString.data(), testString.length());
    file.SetPosition(0);
    const auto clone = file.Clone();
    file.SetPosition(5);
    (void)file.Write("FeelsBadMan", 11);
    SystemAbstractions::IFile::Buffer buffer(testString.length());
    ASSERT_EQ(0, clone->GetPosition());
    ASSERT_EQ(
        testString.length(),
        clone->Read(buffer)
    );
    ASSERT_EQ(
        "HelloFeelsBadMa",
        std::string(
            buffer.begin(),
            buffer.end()
        )
    );
}

TEST_F(FileTests, WriteBeyondEndAndIntoMiddle) {
    const std::string testFilePath = testAreaPath + "/foo.txt";
    SystemAbstractions::File file(testFilePath);
    ASSERT_TRUE(file.Create());
    const std::string testString = "Hello, World!\r\n";
    (void)file.Write(testString.data(), 5);
    file.SetPosition(7);
    (void)file.Write(testString.data() + 7, 8);
    ASSERT_EQ(testString.length(), file.GetSize());
    SystemAbstractions::IFile::Buffer buffer(testString.length());
    file.SetPosition(0);
    ASSERT_EQ(
        testString.length(),
        file.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', 0, 0, 'W', 'o', 'r', 'l', 'd', '!', '\r', '\n'}),
        buffer
    );
    file.SetPosition(5);
    (void)file.Write(testString.data() + 5, 2);
    ASSERT_EQ(testString.length(), file.GetSize());
    file.SetPosition(0);
    ASSERT_EQ(
        testString.length(),
        file.Read(buffer)
    );
    ASSERT_EQ(
        (std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\r', '\n'}),
        buffer
    );
}

TEST_F(FileTests, IsAbsolutePath) {
    struct TestVector {
        std::string path;
        bool isAbsolute;
    };
    const std::vector< TestVector > testVectors{
#ifdef _WIN32
        { "C:/foo", true },
        { "E:/foo", true },
        { "C:\\foo", true },
        { "EE:/foo", false },
        { "E/foo", false },
        { "E\\foo", false },
        { "/foo", false },
        { "\\foo", false },
        { "foo", false },
        { "foo:/", false },
#else
        { "/foo", true },
        { "\\foo", false },
        { "foo", false },
#endif
    };
    for (const auto& testVector: testVectors) {
        EXPECT_EQ(
            testVector.isAbsolute,
            SystemAbstractions::File::IsAbsolutePath(testVector.path)
        );
    }
}
