/**
 * @file SubprocessWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::Subprocess class.
 *
 * © 2016-2018 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>

#include <SystemAbstractions/Subprocess.hpp>
#include <SystemAbstractions/StringExtensions.hpp>

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <thread>

namespace SystemAbstractions {

    /**
     * This structure contains the private methods and properties of
     * the DirectoryMonitor class.
     */
    struct Subprocess::Impl {
        // Properties

        /**
         * @todo Needs documentation
         */
        std::thread worker;

        /**
         * This is a callback to call if the child process
         * exits normally (without crashing).
         */
        std::function< void() > childExited;

        /**
         * This is a callback to call if the child process
         * exits abnormally (crashes).
         */
        std::function< void() > childCrashed;

        /**
         * @todo Needs documentation
         */
        HANDLE child = INVALID_HANDLE_VALUE;

        /**
         * @todo Needs documentation
         */
        HANDLE pipe = INVALID_HANDLE_VALUE;

        /**
         * @todo Needs documentation
         */
        void (*previousSignalHandler)(int) = nullptr;

        // Methods

        /**
         * @todo Needs documentation
         */
        static void SignalHandler(int) {
        }

        /**
         * @todo Needs documentation
         */
        void MonitorChild() {
            previousSignalHandler = signal(SIGINT, SignalHandler);
            (void)WaitForSingleObject(pipe, INFINITE);
            uint8_t token;
            DWORD amtRead = 0;
            if (ReadFile(pipe, &token, 1, &amtRead, NULL) == FALSE) {
                childCrashed();
            } else {
                childExited();
            }
            (void)WaitForSingleObject(child, INFINITE);
            (void)signal(SIGINT, previousSignalHandler);
        }

        /**
         * @todo Needs documentation
         */
        void JoinChild() {
            if (worker.joinable()) {
                worker.join();
                (void)CloseHandle(child);
                child = INVALID_HANDLE_VALUE;
                (void)CloseHandle(pipe);
                pipe = INVALID_HANDLE_VALUE;
            }
        }
    };

    Subprocess::~Subprocess() {
        impl_->JoinChild();
        if (impl_->pipe != INVALID_HANDLE_VALUE) {
            uint8_t token = 42;
            DWORD amtWritten;
            (void)WriteFile(impl_->pipe, &token, 1, &amtWritten, NULL);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            (void)CloseHandle(impl_->pipe);
        }
    }
    Subprocess::Subprocess(Subprocess&&) = default;
    Subprocess& Subprocess::operator=(Subprocess&&) = default;

    Subprocess::Subprocess()
        : impl_(new Impl())
    {
    }

    bool Subprocess::StartChild(
        std::string program,
        const std::vector< std::string >& args,
        std::function< void() > childExited,
        std::function< void() > childCrashed
    ) {
        impl_->JoinChild();
        impl_->childExited = childExited;
        impl_->childCrashed = childCrashed;
        SECURITY_ATTRIBUTES sa;
        (void)memset(&sa, 0, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        HANDLE childPipe;
        if (CreatePipe(&impl_->pipe, &childPipe, &sa, 0) == FALSE) {
            return false;
        }

        // Add file extension because that part is platform-specific.
        program += ".exe";

        std::vector< std::string > childArgs;
        childArgs.push_back("child");
        childArgs.push_back(SystemAbstractions::sprintf("%" PRIu64, (uint64_t)childPipe));
        childArgs.insert(childArgs.end(), args.begin(), args.end());

        // Construct command line of program from argument list.
        // The rules for quoting are a bit complex.  For more information
        // see http://daviddeley.com/autohotkey/parameters/parameters.htm
        std::vector< char > commandLine;
        if (program.find_first_of('"') == std::string::npos) {
            commandLine.insert(commandLine.end(), program.begin(), program.end());
        } else {
            commandLine.push_back('"');
            commandLine.insert(commandLine.end(), program.begin(), program.end());
            commandLine.push_back('"');
        }
        for (const auto arg: childArgs) {
            commandLine.push_back(' ');
            if (arg.find_first_of(" \t\n\v\"") == std::string::npos) {
                commandLine.insert(commandLine.end(), arg.begin(), arg.end());
            } else {
                commandLine.push_back('"');
                int slashCount = 0;
                for (int i = 0; i < arg.length(); ++i) {
                    if (arg[i] == '\\') {
                        ++slashCount;
                    } else {
                        commandLine.insert(commandLine.end(), slashCount, '\\');
                        if (arg[i] == '"') {
                            commandLine.insert(commandLine.end(), slashCount + 1, '\\');
                        }
                        commandLine.push_back(arg[i]);
                        slashCount = 0;
                    }
                }
                if (slashCount > 0) {
                    commandLine.insert(commandLine.end(), slashCount * 2, '\\');
                }
                commandLine.push_back('"');
            }
        }
        commandLine.push_back(0);

        // Launch program.
        STARTUPINFOA si;
        (void)memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi;
        if (
            CreateProcessA(
                program.c_str(),
                &commandLine[0],
                NULL,
                NULL,
                TRUE,
                0,
                NULL,
                NULL,
                &si,
                &pi
            ) == 0
        ) {
            (void)CloseHandle(impl_->pipe);
            impl_->pipe = INVALID_HANDLE_VALUE;
            (void)CloseHandle(childPipe);
            return false;
        }
        impl_->child = pi.hProcess;
        (void)CloseHandle(childPipe);
        impl_->worker = std::thread(&Impl::MonitorChild, impl_.get());
        return true;
    }

    bool Subprocess::ContactParent(std::vector< std::string >& args) {
        if (
            (args.size() >= 2)
            && (args[0] == "child")
        ) {
            uint64_t pipeNumber;
            if (sscanf(args[1].c_str(), "%" SCNu64, &pipeNumber) != 1) {
                return false;
            }
            impl_->pipe = (HANDLE)pipeNumber;
            args.erase(args.begin(), args.begin() + 2);
            return true;
        }
        return false;
    }

}
