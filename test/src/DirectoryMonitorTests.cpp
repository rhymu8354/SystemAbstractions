/**
 * @file DirectoryMonitorTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DirectoryMonitor class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <SystemAbstractions/File.hpp>
#include <fstream>

/**
 * This object implements the required callback interface
 * of the DirectoryMonitor class, in order to receive
 * a callback when a directory changes.
 */
struct DirectoryMonitorOwner
    : public SystemAbstractions::DirectoryMonitor::Owner
{
    // Properties

    /**
     * This flag is set if the DirectoryMonitorChangeDetected
     * callback method is called, indicating a change was made
     * to a directory being monitored.
     */
    bool changeDetected = false;

    // Methods

    // SystemAbstractions::DirectoryMonitor::Owner
public:
    virtual void DirectoryMonitorChangeDetected() override {
        changeDetected = true;
    }
};

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

    /**
     * This object implements the required callback interface
     * of the DirectoryMonitor class, in order to receive
     * a callback when a directory changes.
     */
    DirectoryMonitorOwner dmOwner;

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
    ASSERT_TRUE(dm.Start(&dmOwner, innerPath));
    ASSERT_FALSE(dmOwner.changeDetected);

    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmOwner.changeDetected);
        dmOwner.changeDetected = false;
    }

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmOwner.changeDetected);
        dmOwner.changeDetected = false;
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(dmOwner.changeDetected);
        dmOwner.changeDetected = false;
    }

    // Create a file outside the monitored area.
    testFilePath = outerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmOwner.changeDetected);
    }

    // Edit the file outside the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmOwner.changeDetected);
    }

    // Delete the file outside the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(dmOwner.changeDetected);
    }
}

TEST_F(DirectoryMonitorTests, MoveDirectoryMonitor) {
    // Start monitoring here.
    ASSERT_TRUE(dm.Start(&dmOwner, innerPath));
    ASSERT_FALSE(dmOwner.changeDetected);

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
        ASSERT_TRUE(dmOwner.changeDetected);
        dmOwner.changeDetected = false;
    }

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmOwner.changeDetected);
        dmOwner.changeDetected = false;
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(dmOwner.changeDetected);
        dmOwner.changeDetected = false;
    }

    // Create a file outside the monitored area.
    testFilePath = outerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmOwner.changeDetected);
    }

    // Edit the file outside the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmOwner.changeDetected);
    }

    // Delete the file outside the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(dmOwner.changeDetected);
    }
}

TEST_F(DirectoryMonitorTests, Stop) {
    // Start monitoring here.
    ASSERT_TRUE(dm.Start(&dmOwner, innerPath));
    ASSERT_FALSE(dmOwner.changeDetected);

    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmOwner.changeDetected);
        dmOwner.changeDetected = false;
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
        ASSERT_FALSE(dmOwner.changeDetected);
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(dmOwner.changeDetected);
    }
}
