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

#include <assert.h>
#include <inttypes.h>
#include <IPHlpApi.h>
#include <Psapi.h>
#include <map>
#include <set>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <SystemAbstractions/Subprocess.hpp>
#include <SystemAbstractions/StringExtensions.hpp>
#include <thread>
#pragma comment(lib, "IPHlpApi")

namespace {

    /**
     * This function construct the command line of a program with an argument
     * list.  The rules for quoting are a bit complex.  For more information
     * see http://daviddeley.com/autohotkey/parameters/parameters.htm
     *
     * @param[in] program
     *     This is the path and name of the program.
     *
     * @param[in] args
     *     These are the arguments to pass to the program.
     *
     * @return
     *     The constructed command line is returned.
     */
    std::vector< char > MakeCommandLine(
        const std::string& program,
        const std::vector< std::string >& args
    ) {
        std::vector< char > commandLine;
        if (program.find_first_of('"') == std::string::npos) {
            commandLine.insert(commandLine.end(), program.begin(), program.end());
        } else {
            commandLine.push_back('"');
            commandLine.insert(commandLine.end(), program.begin(), program.end());
            commandLine.push_back('"');
        }
        for (const auto arg: args) {
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
        return commandLine;
    }

    /**
     * This function replaces all backslashes with forward slashes
     * in the given string.
     *
     * @param[in] in
     *     This is the string to fix.
     *
     * @return
     *     A copy of the given string, with all backslashes replaced
     *     with forward slashes, is returned.
     */
    std::string FixPathDelimiters(const std::string& in) {
        std::string out;
        for (auto c: in) {
            if (c == '\\') {
                out.push_back('/');
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

}

namespace SystemAbstractions {

    /**
     * This structure contains the private methods and properties of
     * the DirectoryMonitor class.
     */
    struct Subprocess::Impl {
        // Properties

        /**
         * This is a thread that is run when the Subprocess class
         * is used in the parent role.  The thread basically waits
         * for one of two things to happen:
         * - The child process writes to the pipe, signaling that it's
         *   about to exit normally.
         * - The pipe breaks, indicating that the child process crashed.
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
         * This is the operating system handle to the child process object.
         * It's used to block until the child process is completely cleaned
         * up (after the child process exits or crashes).
         */
        HANDLE child = INVALID_HANDLE_VALUE;

        /**
         * This is one end of the pipe used to communicate between the
         * parent and child processes.  The parent only reads it, to know
         * when the child exits or crashes.  The child writes to it before
         * exiting normally.  The actual data transferred is not important.
         */
        HANDLE pipe = INVALID_HANDLE_VALUE;

        /**
         * This holds onto the previous handler of the SIGINT signal,
         * which is temporarily ignored while monitoring the child process.
         */
        void (*previousSignalHandler)(int) = nullptr;

        // Methods

        /**
         * This is the temporary handler for the SIGINT signal,
         * while monitoring the child process.
         */
        static void SignalHandler(int) {
        }

        /**
         * This is the body of the worker thread that is run
         * to wait for the child to either exit normally or crash.
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
         * This method is called in three different use cases:
         * - When the child process is exiting normally, in which case
         *   it does nothing.
         * - When the parent process is exiting, in which case it
         *   joins the monitor/worker thread, in order to ensure
         *   the child process exits first.
         * - When the parent process is about to start a new child
         *   process, in which case we need to make sure any previous
         *   child process has exited/crashed, and we close the old pipe.
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

    Subprocess::~Subprocess() noexcept {
        impl_->JoinChild();
        if (impl_->pipe != INVALID_HANDLE_VALUE) {
            uint8_t token = 42;
            DWORD amtWritten;
            (void)WriteFile(impl_->pipe, &token, 1, &amtWritten, NULL);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            (void)CloseHandle(impl_->pipe);
        }
    }
    Subprocess::Subprocess(Subprocess&&) noexcept = default;
    Subprocess& Subprocess::operator=(Subprocess&&) noexcept = default;

    Subprocess::Subprocess()
        : impl_(new Impl())
    {
    }

    unsigned int Subprocess::StartChild(
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
            return 0;
        }

        // Add file extension because that part is platform-specific.
        if (
            (program.length() < 4)
            || (program.substr(program.length() - 4) != ".exe")
        ) {
            program += ".exe";
        }

        std::vector< std::string > childArgs;
        childArgs.push_back("child");
        childArgs.push_back(SystemAbstractions::sprintf("%" PRIu64, (uint64_t)childPipe));
        childArgs.insert(childArgs.end(), args.begin(), args.end());

        auto commandLine = MakeCommandLine(program, childArgs);

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
            return 0;
        }
        impl_->child = pi.hProcess;
        (void)CloseHandle(childPipe);
        impl_->worker = std::thread(&Impl::MonitorChild, impl_.get());
        return (unsigned int)pi.dwProcessId;
    }

    unsigned int Subprocess::StartDetached(
        std::string program,
        const std::vector< std::string >& args
    ) {
        // Add file extension because that part is platform-specific.
        if (
            (program.length() < 4)
            || (program.substr(program.length() - 4) != ".exe")
        ) {
            program += ".exe";
        }

        auto commandLine = MakeCommandLine(program, args);

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
                FALSE,
                DETACHED_PROCESS,
                NULL,
                NULL,
                &si,
                &pi
            ) == 0
        ) {
            return 0;
        }
        (void)CloseHandle(pi.hProcess);
        return (unsigned int)pi.dwProcessId;
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

    unsigned int Subprocess::GetCurrentProcessId() {
        return (unsigned int)::GetCurrentProcessId();
    }

    auto Subprocess::GetProcessList() -> std::vector< ProcessInfo > {
        // Gather the identifiers of all currently running processes.
        std::vector< DWORD > processIds(1024);
        for (;;) {
            DWORD bytesNeeded;
            if (
                EnumProcesses(
                    processIds.data(),
                    (DWORD)(processIds.size() * sizeof(DWORD)),
                    &bytesNeeded
                ) == FALSE
            ) {
                return {};
            }
            if (bytesNeeded == processIds.size() * sizeof(DWORD)) {
                processIds.resize(processIds.size() * 2);
            } else {
                processIds.resize(bytesNeeded / sizeof(DWORD));
                break;
            }
        }

        // Gather collections of network ports currently bound in the system.
        ULONG requiredTcpTableSize = 4096;
        std::vector< uint8_t > tcpTableBuffer;
        ULONG getTcpTableResult;
        do {
            tcpTableBuffer.resize(requiredTcpTableSize);
            getTcpTableResult = GetTcpTable2(
                (PMIB_TCPTABLE2)tcpTableBuffer.data(),
                &requiredTcpTableSize,
                FALSE
            );
        } while (getTcpTableResult == ERROR_INSUFFICIENT_BUFFER);
        std::map< unsigned int, std::set< uint16_t > > tcpServerPorts;
        if (getTcpTableResult == NO_ERROR) {
            const auto tcpTable = (PMIB_TCPTABLE2)tcpTableBuffer.data();
            for (size_t i = 0; i < (size_t)tcpTable->dwNumEntries; ++i) {
                const auto& tcpTableEntry = tcpTable->table[i];
                if (tcpTableEntry.dwState == MIB_TCP_STATE_LISTEN) {
                    (void)tcpServerPorts[(unsigned int)tcpTableEntry.dwOwningPid].insert(
                        (
                            (uint16_t)((tcpTableEntry.dwLocalPort >> 8) & 0x00FF)
                            | (uint16_t)((tcpTableEntry.dwLocalPort << 8) & 0xFF00)
                        )
                    );
                }
            }
        }

        // For each process ID, attempt to access the process to get its image
        // executable path.  If successful, add the process information to the
        // returned collection.
        std::vector< ProcessInfo > processes;
        processes.reserve(processIds.size());
        for (size_t i = 0; i < processIds.size(); ++i) {
            ProcessInfo process;
            process.id = (unsigned int)processIds[i];
            const auto processHandle = OpenProcess(
                (
                    PROCESS_QUERY_INFORMATION
                    | PROCESS_VM_READ
                ),
                FALSE,
                processIds[i]
            );
            if (processHandle == NULL) {
                continue;
            }
            HMODULE processModule;
            DWORD processModuleSize;
            if (
                EnumProcessModules(
                    processHandle,
                    &processModule,
                    sizeof(processModule),
                    &processModuleSize
                ) == FALSE
            ) {
                (void)CloseHandle(processHandle);
                continue;
            }
            std::vector< char > exeImagePath(MAX_PATH + 1);
            (void)GetModuleFileNameExA(
                processHandle,
                processModule,
                &exeImagePath[0],
                (DWORD)exeImagePath.size()
            );
            process.image = FixPathDelimiters(exeImagePath.data());
            auto tcpServerPortsEntry = tcpServerPorts.find(process.id);
            if (tcpServerPortsEntry != tcpServerPorts.end()) {
                process.tcpServerPorts = tcpServerPortsEntry->second;
            }
            processes.push_back(std::move(process));
            (void)CloseHandle(processHandle);
        }
        return processes;
    }

    void Subprocess::Kill(unsigned int id) {
        const auto processHandle = OpenProcess(
            (
                PROCESS_TERMINATE
            ),
            FALSE,
            (DWORD)id
        );
        if (processHandle == NULL) {
            return;
        }
        (void)TerminateProcess(processHandle, 255);
        (void)CloseHandle(processHandle);
    }

}
