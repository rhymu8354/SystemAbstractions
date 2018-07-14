/**
 * @file SubprocessTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::Subprocess class.
 *
 * © 2018 by Richard Walters
 */

#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <SystemAbstractions/Subprocess.hpp>
#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <SystemAbstractions/File.hpp>

namespace {

    /**
     * This is used to receive callbacks from the unit under test.
     */
    struct Owner {
        // Properties

        /**
         * This is used to wait for, or signal, a condition
         * upon which that the owner might be waiting.
         */
        std::condition_variable_any condition;

        /**
         * This is used to synchronize access to the class.
         */
        std::mutex mutex;

        /**
         * This flag indicates whether or not the subprocess exited.
         */
        bool exited = false;

        /**
         * This flag indicates whether or not the subprocess crashed.
         */
        bool crashed = false;

        // Methods

        /**
         * This method waits up to a second for the subprocess to exit.
         *
         * @return
         *     An indication of whether or not the subprocess exited is returned.
         */
        bool AwaitExited() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return exited;
                }
            );
        }

        /**
         * This method waits up to a second for the subprocess to crash.
         *
         * @return
         *     An indication of whether or not the subprocess crashed is returned.
         */
        bool AwaitCrashed() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return crashed;
                }
            );
        }

        // SystemAbstractions::Subprocess::Owner

        void SubprocessChildExited() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            exited = true;
            condition.notify_all();
        }

        void SubprocessChildCrashed() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            crashed = true;
            condition.notify_all();
        }
    };

}

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct SubprocessTests
    : public ::testing::Test
{
    // Properties

    /**
     * This is the temporary directory to use to test
     * the File class.
     */
    std::string testAreaPath;

    /**
     * This is used to monitor the temporary directory for changes.
     */
    SystemAbstractions::DirectoryMonitor monitor;

    /**
     * This flag is set if any change happens in the temporary directory.
     */
    bool testAreaChanged = false;

    /**
     * This is used to wait for, or signal, a condition
     * upon which that the test fixture might be waiting.
     */
    std::condition_variable_any condition;

    /**
     * This is used to synchronize access to the text fixture.
     */
    std::mutex mutex;

    // Methods

    /**
     * This method waits up to a second for a change to happen
     * in the test area directory.
     *
     * @return
     *     An indication of whether or not a change happened
     *     in the test area directory is returned.
     */
    bool AwaitTestAreaChanged() {
        std::unique_lock< decltype(mutex) > lock(mutex);
        return condition.wait_for(
            lock,
            std::chrono::seconds(1),
            [this]{
                return testAreaChanged;
            }
        );
    }

    // ::testing::Test

    virtual void SetUp() {
        testAreaPath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(testAreaPath));
        monitor.Start(
            [this]{
                std::lock_guard< decltype(mutex) > lock(mutex);
                testAreaChanged = true;
                condition.notify_all();
            },
            testAreaPath
        );
    }

    virtual void TearDown() {
        monitor.Stop();
        ASSERT_TRUE(SystemAbstractions::File::DeleteDirectory(testAreaPath));
    }
};

TEST_F(SubprocessTests, StartSubprocess) {
    Owner owner;
    SystemAbstractions::Subprocess child;
    ASSERT_TRUE(
        child.StartChild(
            SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocess",
            {"Hello, World", "exit"},
            [&owner]{ owner.SubprocessChildExited(); },
            [&owner]{ owner.SubprocessChildCrashed(); }
        )
    );
    ASSERT_TRUE(AwaitTestAreaChanged());
}

TEST_F(SubprocessTests, Exit) {
    Owner owner;
    SystemAbstractions::Subprocess child;
    child.StartChild(
        SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocess",
        {"Hello, World", "exit"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_TRUE(owner.AwaitExited());
    ASSERT_FALSE(owner.crashed);
}

TEST_F(SubprocessTests, Crash) {
    Owner owner;
    SystemAbstractions::Subprocess child;
    child.StartChild(
        SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocess",
        {"Hello, World", "crash"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_TRUE(owner.AwaitCrashed());
    ASSERT_FALSE(owner.exited);
}
