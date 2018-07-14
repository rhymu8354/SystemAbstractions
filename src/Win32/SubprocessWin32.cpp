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
    struct SubprocessImpl {
        // Properties

        /**
         * @todo Needs documentation
         */
        std::thread worker;

        /**
         * @todo Needs documentation
         */
        Subprocess::Owner* owner = nullptr;

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
                owner->SubprocessChildCrashed();
            } else {
                owner->SubprocessChildExited();
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

    Subprocess::Subprocess()
        : _impl(new SubprocessImpl())
    {
    }


    Subprocess::Subprocess(Subprocess&& other) noexcept
        : _impl(std::move(other._impl))
    {
    }

    Subprocess::Subprocess(std::unique_ptr< SubprocessImpl >&& impl) noexcept
        : _impl(std::move(impl))
    {
    }

    Subprocess::~Subprocess() {
        _impl->JoinChild();
        if (_impl->pipe != INVALID_HANDLE_VALUE) {
            uint8_t token = 42;
            DWORD amtWritten;
            (void)WriteFile(_impl->pipe, &token, 1, &amtWritten, NULL);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            (void)CloseHandle(_impl->pipe);
        }
    }

    Subprocess& Subprocess::operator=(Subprocess&& other) noexcept {
        _impl = std::move(other._impl);
        return *this;
    }

    bool Subprocess::StartChild(
        std::string program,
        const std::vector< std::string >& args,
        Owner* owner
    ) {
        _impl->JoinChild();
        _impl->owner = owner;
        SECURITY_ATTRIBUTES sa;
        (void)memset(&sa, 0, sizeof(sa));
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        HANDLE childPipe;
        if (CreatePipe(&_impl->pipe, &childPipe, &sa, 0) == FALSE) {
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
            (void)CloseHandle(_impl->pipe);
            _impl->pipe = INVALID_HANDLE_VALUE;
            (void)CloseHandle(childPipe);
            return false;
        }
        _impl->child = pi.hProcess;
        (void)CloseHandle(childPipe);
        _impl->worker = std::move(std::thread(&SubprocessImpl::MonitorChild, _impl.get()));
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
            _impl->pipe = (HANDLE)pipeNumber;
            args.erase(args.begin(), args.begin() + 2);
            return true;
        }
        return false;
    }

}
