/**
 * @file DirectoryMonitorTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DirectoryMonitor class.
 *
 * © 2018 by Richard Walters
 */

#include <condition_variable>
#include <functional>
#include <fstream>
#include <gtest/gtest.h>
#include <mutex>
#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <SystemAbstractions/File.hpp>

/**
 * This is a helper used with a directory monitor to
 * receive the "changed" callback and wait for it to
 * happen without racing the directory monitor.
 */
struct ChangedCallbackHelper {
    // Properties

    /**
     * This flag is set when the directory monitor
     * publishes an event to say the directory was changed.
     */
    bool changeDetected = false;

    /**
     * This is used to wait for the change callback to happen.
     */
    std::condition_variable changeCondition;

    /**
     * This is used to synchronize access to this object.
     */
    std::mutex changeMutex;

    /**
     * This is the delegate to be given to the directory
     * monitor in order to hook this helper up.
     */
    std::function< void() > dmCallback = [this]{
        std::lock_guard< std::mutex > lock(changeMutex);
        changeDetected = true;
        changeCondition.notify_all();
    };

    // Methods

    /**
     * This method is called by the unit tests in order to
     * wait for the directory monitor callback to happen.
     *
     * @return
     *     An indication of whether or not the callback
     *     happened before a reasonable amount of time
     *     has elapsed is returned.
     */
    bool AwaitChanged() {
        std::unique_lock< decltype(changeMutex) > lock(changeMutex);
        const auto changed = changeCondition.wait_for(
            lock,
            std::chrono::milliseconds(50),
            [this]{ return changeDetected; }
        );
        changeDetected = false;
        return changed;
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
     * This used to wait for callbacks.
     */
    ChangedCallbackHelper dmCallbackHelper;

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

TEST_F(DirectoryMonitorTests, NoCallbackJustAfterStartingMonitor) {
    // Start monitoring here.
    ASSERT_TRUE(dm.Start(dmCallbackHelper.dmCallback, innerPath));
    ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
}

TEST_F(DirectoryMonitorTests, DirectoryMonitoring) {
    // Start monitoring here.
    ASSERT_TRUE(dm.Start(dmCallbackHelper.dmCallback, innerPath));

    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }

    // Create a file outside the monitored area.
    testFilePath = outerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
    }

    // Edit the file outside the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
    }

    // Delete the file outside the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
    }
}

TEST_F(DirectoryMonitorTests, MoveDirectoryMonitor) {
    // Start monitoring here.
    ASSERT_TRUE(dm.Start(dmCallbackHelper.dmCallback, innerPath));

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
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }

    // Create a file outside the monitored area.
    testFilePath = outerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
    }

    // Edit the file outside the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
    }

    // Delete the file outside the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
    }
}

TEST_F(DirectoryMonitorTests, Stop) {
    // Start monitoring here.
    ASSERT_TRUE(dm.Start(dmCallbackHelper.dmCallback, innerPath));

    // Create a file in the monitored area.
    std::string testFilePath = innerPath + "/fred.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
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
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(dmCallbackHelper.AwaitChanged());
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
    ASSERT_TRUE(dm.Start(dmCallbackHelper.dmCallback, innerPath));

    // Edit the file in the monitored area.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }

    // Delete the file in the monitored area.
    {
        SystemAbstractions::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(dmCallbackHelper.AwaitChanged());
    }
}

TEST_F(DirectoryMonitorTests, MoveBeforeStart) {
    SystemAbstractions::DirectoryMonitor newDm(std::move(dm));
    ASSERT_FALSE(dm.Start(dmCallbackHelper.dmCallback, innerPath));
}
