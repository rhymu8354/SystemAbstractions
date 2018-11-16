/**
 * @file SubprocessTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::Subprocess class.
 *
 * © 2018 by Richard Walters
 */

#include <condition_variable>
#include <fstream>
#include <gtest/gtest.h>
#include <mutex>
#include <stdio.h>
#include <string>
#include <SystemAbstractions/Subprocess.hpp>
#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/NetworkEndpoint.hpp>
#include <thread>
#include <vector>

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
    const auto reportedPid = child.StartChild(
        SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocessProgram",
        {"Hello, World", "exit"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_NE(0, reportedPid);
    ASSERT_TRUE(AwaitTestAreaChanged());
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    const std::string pidFilePath = testAreaPath + "/pid";
    FILE* pidFile = fopen(pidFilePath.c_str(), "r");
    ASSERT_FALSE(pidFile == NULL);
    unsigned int pid;
    const auto pidScanResult = fscanf(pidFile, "%u", &pid);
    (void)fclose(pidFile);
    ASSERT_EQ(1, pidScanResult);
    EXPECT_EQ(pid, reportedPid);
}

#ifdef _WIN32
TEST_F(SubprocessTests, StartSubprocessWithFileExtension) {
    Owner owner;
    SystemAbstractions::Subprocess child;
    ASSERT_TRUE(
        child.StartChild(
            SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocessProgram.exe",
            {"Hello, World", "exit"},
            [&owner]{ owner.SubprocessChildExited(); },
            [&owner]{ owner.SubprocessChildCrashed(); }
        ) != 0
    );
    ASSERT_TRUE(AwaitTestAreaChanged());
}
#endif /* _WIN32 */

#ifndef _WIN32
TEST_F(SubprocessTests, FileHandlesNotInherited) {
    Owner owner;
    SystemAbstractions::Subprocess child;
    (void)child.StartChild(
        SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocessProgram",
        {"Hello, World", "handles"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    (void)owner.AwaitExited();
    SystemAbstractions::File handlesReport(testAreaPath + "/handles");
    ASSERT_TRUE(handlesReport.Open());
    std::vector< char > handles(handlesReport.GetSize());
    (void)handlesReport.Read(handles.data(), handles.size());
    EXPECT_EQ(0, handles.size()) << std::string(handles.begin(), handles.end());
}
#endif /* not _WIN32 */

TEST_F(SubprocessTests, Exit) {
    Owner owner;
    SystemAbstractions::Subprocess child;
    (void)child.StartChild(
        SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocessProgram",
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
    (void)child.StartChild(
        SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocessProgram",
        {"Hello, World", "crash"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_TRUE(owner.AwaitCrashed());
    ASSERT_FALSE(owner.exited);
}

TEST_F(SubprocessTests, Detached) {
    // Start the detached process.
    std::vector< std::string > args{
        "detached",
    };
    const auto reportedPid = SystemAbstractions::Subprocess::StartDetached(
        SystemAbstractions::File::GetExeParentDirectory() + "/MockSubprocessProgram",
        args
    );
    ASSERT_NE(0, reportedPid);

    // Wait a short period of time so that we don't race the detached process.
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Verify process ID matches what the detached process says it has.
    const std::string pidFilePath = testAreaPath + "/pid";
    FILE* pidFile = fopen(pidFilePath.c_str(), "r");
    ASSERT_FALSE(pidFile == NULL);
    unsigned int pid;
    const auto pidScanResult = fscanf(pidFile, "%u", &pid);
    (void)fclose(pidFile);
    ASSERT_EQ(1, pidScanResult);
    EXPECT_EQ(pid, reportedPid);

    // Verify command-line arguments given to the detached process match what
    // it says it received.
    std::ifstream foo(testAreaPath + "/foo.txt");
    ASSERT_TRUE(foo.is_open());
    std::vector< std::string > lines;
    while (!foo.eof() && !foo.fail()) {
        std::string line;
        (void)std::getline(foo, line);
        if (
            !line.empty()
            || !foo.eof()
        ) {
            lines.push_back(std::move(line));
        }
    }
    EXPECT_EQ(args, lines);

#ifndef _WIN32
    // Linux targets also know what file handles are open, so check to make
    // sure none are in the detached process.
    SystemAbstractions::File handlesReport(testAreaPath + "/handles");
    ASSERT_TRUE(handlesReport.Open());
    std::vector< char > handles(handlesReport.GetSize());
    (void)handlesReport.Read(handles.data(), handles.size());
    EXPECT_EQ(0, handles.size()) << std::string(handles.begin(), handles.end());
#endif /* _WIN32 */
}

TEST_F(SubprocessTests, FindSelfByImagePath) {
    const auto processes = SystemAbstractions::Subprocess::GetProcessList();
    const auto selfPath = SystemAbstractions::File::GetExeImagePath();
    bool foundSelf = false;
    for (const auto& process: processes) {
        if (process.image == selfPath) {
            if (process.id == SystemAbstractions::Subprocess::GetCurrentProcessId()) {
                foundSelf = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundSelf);
}

TEST_F(SubprocessTests, FindSelfByTcpServerPort) {
    SystemAbstractions::NetworkEndpoint tcp;
    ASSERT_TRUE(
        tcp.Open(
            [](
                std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
            ){},
            [](
                uint32_t address,
                uint16_t port,
                const std::vector< uint8_t >& body
            ){},
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0, 0, 0
        )
    );
    const auto processes = SystemAbstractions::Subprocess::GetProcessList();
    const auto selfPath = SystemAbstractions::File::GetExeImagePath();
    bool foundSelf = false;
    for (const auto& process: processes) {
        for (const auto port: process.tcpServerPorts) {
            if (port == tcp.GetBoundPort()) {
                EXPECT_EQ(
                    SystemAbstractions::Subprocess::GetCurrentProcessId(),
                    process.id
                );
                foundSelf = true;
                break;
            }
        }
        if (foundSelf) {
            break;
        }
    }
    EXPECT_TRUE(foundSelf);
}
