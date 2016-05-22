/**
 * @file DirectoryMonitorLinux.cpp
 *
 * This module contains the Linux implementation of the
 * Files::DirectoryMonitor class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../DirectoryMonitor.hpp"
#include "PipeSignal.hpp"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace Files {

    /**
     * This structure contains the private methods and properties of
     * the DirectoryMonitor class.
     */
    struct DirectoryMonitorImpl {
        // Properties

        /**
         * @todo Needs documentation
         */
        std::thread worker;

        /**
         * @todo Needs documentation
         */
        DirectoryMonitor::Owner* owner;

        /**
         * @todo Needs documentation
         */
        int inotifyQueue = -1;

        /**
         * @todo Needs documentation
         */
        int inotifyWatch = -1;

        /**
         * @todo Needs documentation
         */
        PipeSignal stopSignal;

        // Methods

        /**
         * @todo Needs documentation
         */
        void Run() {
            const int stopSelectHandle = stopSignal.GetSelectHandle();
            const int nfds = std::max(stopSelectHandle, inotifyQueue) + 1;
            fd_set readfds;
            std::vector< uint8_t > buffer(65536);
            for (;;) {
                FD_ZERO(&readfds);
                FD_SET(stopSelectHandle, &readfds);
                FD_SET(inotifyQueue, &readfds);
                (void)select(nfds, &readfds, NULL, NULL, NULL);
                if (FD_ISSET(stopSelectHandle, &readfds)) {
                    break;
                }
                if (FD_ISSET(inotifyQueue, &readfds)) {
                    while (read(inotifyQueue, &buffer[0], buffer.size()) > 0) {
                    }
                    owner->DirectoryMonitorChangeDetected();
                }
            }
        }
    };

    DirectoryMonitor::DirectoryMonitor()
        : _impl(new DirectoryMonitorImpl())
    {
    }


    DirectoryMonitor::DirectoryMonitor(DirectoryMonitor&& other)
        : _impl(std::move(other._impl))
    {
    }

    DirectoryMonitor::DirectoryMonitor(std::unique_ptr< DirectoryMonitorImpl >&& impl)
        : _impl(std::move(impl))
    {
    }

    DirectoryMonitor::~DirectoryMonitor() {
        Stop();
    }

    DirectoryMonitor& DirectoryMonitor::operator=(DirectoryMonitor&& other) {
        assert(this != &other);
        _impl = std::move(other._impl);
        return *this;
    }

    bool DirectoryMonitor::Start(
        Owner* owner,
        const std::string& path
    ) {
        Stop();
        if (!_impl->stopSignal.Initialize()) {
            return false;
        }
        _impl->stopSignal.Clear();
        _impl->owner = owner;
        _impl->inotifyQueue = inotify_init();
        if (_impl->inotifyQueue < 0) {
            return false;
        }
        int flags = fcntl(_impl->inotifyQueue, F_GETFL, 0);
        if (flags < 0) {
            (void)close(_impl->inotifyQueue);
            _impl->inotifyQueue = -1;
            return false;
        }
        flags |= O_NONBLOCK;
        if (fcntl(_impl->inotifyQueue, F_SETFL, flags) < 0) {
            (void)close(_impl->inotifyQueue);
            _impl->inotifyQueue = -1;
            return false;
        }
        _impl->inotifyWatch = inotify_add_watch(
            _impl->inotifyQueue,
            path.c_str(),
            IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO
        );
        if (_impl->inotifyWatch < 0) {
            (void)close(_impl->inotifyQueue);
            _impl->inotifyQueue = -1;
        }
        _impl->worker = std::move(std::thread(&DirectoryMonitorImpl::Run, _impl.get()));
        return true;
    }

    void DirectoryMonitor::Stop() {
        if (!_impl->worker.joinable()) {
            return;
        }
        _impl->stopSignal.Set();
        _impl->worker.join();
        _impl->inotifyWatch = -1;
        (void)close(_impl->inotifyQueue);
        _impl->inotifyQueue = -1;
    }

}
