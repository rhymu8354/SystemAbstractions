/**
 * @file PipeSignal.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::PipeSignal class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "PipeSignal.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

namespace SystemAbstractions {

    /**
     * This contains the private properties and methods of the class.
     */
    struct PipeSignalImpl {
        /**
         * This is the pipe which is used to carry the state of
         * the signal.
         */
        int pipe[2] = {-1, -1};

        /**
         * This is a human-readable string indicating the last error that
         * occurred in another method of the instance.
         */
        std::string lastError;
    };

    PipeSignal::PipeSignal()
        : _impl(new PipeSignalImpl())
    {
    }

    PipeSignal::~PipeSignal() {
        if (_impl->pipe[1] >= 0) {
            (void)close(_impl->pipe[1]);
        }
        if (_impl->pipe[0] >= 0) {
            (void)close(_impl->pipe[0]);
        }
    }

    bool PipeSignal::Initialize() {
        if (_impl->pipe[0] >= 0) {
            return true;
        }
        if (pipe2(_impl->pipe, O_NONBLOCK) != 0) {
            _impl->lastError = strerror(errno);
            _impl->pipe[0] = -1;
            _impl->pipe[1] = -1;
            return false;
        }
        return true;
    }

    std::string PipeSignal::GetLastError() const {
        return _impl->lastError;
    }

    void PipeSignal::Set() {
        uint8_t token = 46; // '.' character, because why not?
        (void)write(_impl->pipe[1], &token, 1);
    }

    void PipeSignal::Clear() {
        uint8_t token;
        (void)read(_impl->pipe[0], &token, 1);
    }

    bool PipeSignal::IsSet() const {
        fd_set readfds;
        struct timeval timeout = {0};
        FD_ZERO(&readfds);
        FD_SET(_impl->pipe[0], &readfds);
        return (select(_impl->pipe[0] + 1, &readfds, NULL, NULL, &timeout) != 0);
    }

    int PipeSignal::GetSelectHandle() const {
        return _impl->pipe[0];
    }

}
