/**
 * @file SubprocessPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::Subprocess class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <SystemAbstractions/Subprocess.hpp>
#include <SystemAbstractions/StringExtensions.hpp>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

    std::vector< char > VectorFromString(const std::string& s) {
        std::vector< char > v(s.length() + 1);
        for (size_t i = 0; i < s.length(); ++i) {
            v[i] = s[i];
        }
        return v;
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
         * @todo Needs documentation
         */
        pid_t child = -1;

        /**
         * @todo Needs documentation
         */
        int pipe = -1;

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
            for (;;) {
                uint8_t token;
                auto amtRead = read(pipe, &token, 1);
                if (amtRead > 0) {
                    childExited();
                    break;
                } else if (
                    (amtRead == 0)
                    || (
                        (amtRead < 0)
                        && (errno != EINTR)
                    )
                ) {
                    childCrashed();
                    break;
                }
            }
            (void)waitpid(child, NULL, 0);
            (void)signal(SIGINT, previousSignalHandler);
        }

        /**
          * @todo Needs documentation
          */
        void JoinChild() {
            if (worker.joinable()) {
                worker.join();
                child = -1;
                (void)close(pipe);
                pipe = -1;
            }
        }
    };

    Subprocess::~Subprocess() noexcept {
        impl_->JoinChild();
        if (impl_->pipe >= 0) {
            uint8_t token = 42;
            (void)write(impl_->pipe, &token, 1);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            (void)close(impl_->pipe);
        }
    }
    Subprocess::Subprocess(Subprocess&&) noexcept = default;
    Subprocess& Subprocess::operator=(Subprocess&&) noexcept = default;

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
        int pipeEnds[2];
        if (pipe(pipeEnds) < 0) {
            return false;
        }

        std::vector< std::vector< char > > childArgs;
        childArgs.emplace_back(VectorFromString(program));
        childArgs.emplace_back(VectorFromString("child"));
        childArgs.emplace_back(VectorFromString(SystemAbstractions::sprintf("%d", pipeEnds[1])));
        for (const auto arg: args) {
            childArgs.emplace_back(VectorFromString(arg));
        }

        // Launch program.
        impl_->child = fork();
        if (impl_->child == 0) {
            (void)close(pipeEnds[0]);
            std::vector< char* > argv(childArgs.size() + 1);
            for (size_t i = 0; i < childArgs.size(); ++i) {
                argv[i] = &childArgs[i][0];
            }
            argv[childArgs.size() + 1] = NULL;
            (void)execv(program.c_str(), &argv[0]);
            (void)exit(-1);
        } else if (impl_->child < 0) {
            (void)close(pipeEnds[0]);
            (void)close(pipeEnds[1]);
            return false;
        }
        impl_->pipe = pipeEnds[0];
        (void)close(pipeEnds[1]);
        impl_->worker = std::thread(&Impl::MonitorChild, impl_.get());
        return true;
    }

    bool Subprocess::ContactParent(std::vector< std::string >& args) {
        if (
            (args.size() >= 2)
            && (args[0] == "child")
        ) {
            int pipeNumber;
            if (sscanf(args[1].c_str(), "%d", &pipeNumber) != 1) {
                return false;
            }
            impl_->pipe = pipeNumber;
            args.erase(args.begin(), args.begin() + 2);
            return true;
        }
        return false;
    }

}
