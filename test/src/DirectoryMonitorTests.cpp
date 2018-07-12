/**
 * @file DirectoryMonitorTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DirectoryMonitor class.
 *
 * © 2018 by Richard Walters
 */

#include <functional>
#include <gtest/gtest.h>
#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <SystemAbstractions/File.hpp>
#include <fstream>

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct DirectoryMonitorTests
    : public ::testing::Test
{
    // Properties

    /**
     * This is the unit under test.
     */
    SystemAbstractions::DirectoryMonitor dm;

    /**
     * This is the temporary directory to use to test
     * the DirectoryMonitor class.
     */
    std::string outerPath;

    /**
     * This is the directory to be monitored for changes.
     * It will be a subdirectory of outerPath.
     */
    std::string innerPath;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
        outerPath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea";
        innerPath = outerPath + "/MonitorThis";
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(outerPath));
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(innerPath));
    }

    virtual void TearDown() {
        dm.Stop();
        ASSERT_TRUE(SystemAbstractions::File::DeleteDirectory(outerPath));
    }
};

TEST_F(DirectoryMonitorTests, DirectoryMonitoring) {
    // Start monitoring here.
    bool changeDetected = false;
    std::function< void() > dmCallback = [&changeDetected]{ changeDetected = true; };
    ASSERT_TRUE(dm.Start(dmCallback, innerPath));
    ASSERT_FALSE(changeDetected);

    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Create a file outside the monitored area.
    testFilePath = outerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(changeDetected);
    }

    // Edit the file outside the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(changeDetected);
    }

    // Delete the file outside the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(changeDetected);
    }
}

TEST_F(DirectoryMonitorTests, MoveDirectoryMonitor) {
    // Start monitoring here.
    bool changeDetected = false;
    std::function< void() > dmCallback = [&changeDetected]{ changeDetected = true; };
    ASSERT_TRUE(dm.Start(dmCallback, innerPath));
    ASSERT_FALSE(changeDetected);

    // Move the monitor
    SystemAbstractions::DirectoryMonitor newDm(std::move(dm));
    dm.Stop();
    dm = std::move(newDm);
    newDm.Stop();

    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Create a file outside the monitored area.
    testFilePath = outerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(changeDetected);
    }

    // Edit the file outside the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(changeDetected);
    }

    // Delete the file outside the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(changeDetected);
    }
}

TEST_F(DirectoryMonitorTests, Stop) {
    // Start monitoring here.
    bool changeDetected = false;
    std::function< void() > dmCallback = [&changeDetected]{ changeDetected = true; };
    ASSERT_TRUE(dm.Start(dmCallback, innerPath));
    ASSERT_FALSE(changeDetected);

    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Stop monitoring
    dm.Stop();

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(changeDetected);
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(changeDetected);
    }
}

TEST_F(DirectoryMonitorTests, ChangeFileThatExistedBeforeMonitoringBegan) {
    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
    }

    // Start monitoring here.
    bool changeDetected = false;
    std::function< void() > dmCallback = [&changeDetected]{ changeDetected = true; };
    ASSERT_TRUE(dm.Start(dmCallback, innerPath));
    ASSERT_FALSE(changeDetected);

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(changeDetected);
        changeDetected = false;
    }
}
