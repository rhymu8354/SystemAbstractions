/**
 * @file SubprocessPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::Subprocess class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../Subprocess.hpp"
#include "../StringExtensions.hpp"

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
    struct SubprocessImpl {
        // Properties

        /**
         * @todo Needs documentation
         */
        std::thread worker;

        /**
         * @todo Needs documentation
         */
        Subprocess::Owner* owner;

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
                    owner->SubprocessChildExited();
                    break;
                } else if (
                    (amtRead == 0)
                    || (
                        (amtRead < 0)
                        && (errno != EINTR)
                    )
                ) {
                    owner->SubprocessChildCrashed();
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

    Subprocess::Subprocess()
        : _impl(new SubprocessImpl())
    {
    }


    Subprocess::Subprocess(Subprocess&& other)
        : _impl(std::move(other._impl))
    {
    }

    Subprocess::Subprocess(std::unique_ptr< SubprocessImpl >&& impl)
        : _impl(std::move(impl))
    {
    }

    Subprocess::~Subprocess() {
        _impl->JoinChild();
        if (_impl->pipe >= 0) {
            uint8_t token = 42;
            (void)write(_impl->pipe, &token, 1);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            (void)close(_impl->pipe);
        }
    }

    Subprocess& Subprocess::operator=(Subprocess&& other) {
        assert(this != &other);
        _impl = std::move(other._impl);
        return *this;
    }

    bool Subprocess::StartChild(
        const std::string& program,
        const std::vector< std::string >& args,
        Owner* owner
    ) {
        _impl->JoinChild();
        _impl->owner = owner;
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
        _impl->child = fork();
        if (_impl->child == 0) {
            (void)close(pipeEnds[0]);
            std::vector< char* > argv(childArgs.size() + 1);
            for (size_t i = 0; i < childArgs.size(); ++i) {
                argv[i] = &childArgs[i][0];
            }
            argv[childArgs.size() + 1] = NULL;
            (void)execv(program.c_str(), &argv[0]);
            (void)exit(-1);
        } else if (_impl->child < 0) {
            (void)close(pipeEnds[0]);
            (void)close(pipeEnds[1]);
            return false;
        }
        _impl->pipe = pipeEnds[0];
        (void)close(pipeEnds[1]);
        _impl->worker = std::move(std::thread(&SubprocessImpl::MonitorChild, _impl.get()));
        return true;
    }

    bool Subprocess::ContactParent(
        std::vector< std::string >& args,
        Owner* owner
    ) {
        _impl->owner = owner;
        if (
            (args.size() >= 2)
            && (args[0] == "child")
        ) {
            int pipeNumber;
            if (sscanf(args[1].c_str(), "%d", &pipeNumber) != 1) {
                return false;
            }
            _impl->pipe = pipeNumber;
            args.erase(args.begin(), args.begin() + 2);
            return true;
        }
        return false;
    }

}
