/**
 * @file DirectoryMonitorMach.cpp
 *
 * This module contains the Mach implementation of the
 * SystemAbstractions::DirectoryMonitor class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../DirectoryMonitor.hpp"
#include "../Posix/PipeSignal.hpp"

#include <assert.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace SystemAbstractions {

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
        PipeSignal stopSignal;

        /**
         * @todo Needs documentation
         */
        int dirHandle = -1;

        /**
         * @todo Needs documentation
         */
        int kqueueHandle = -1;

        /**
         * @todo Needs documentation
         */

        // Methods

        /**
         * @todo Needs documentation
         */
        void Run() {
            struct kevent changes[2];
            EV_SET(&changes[0], stopSignal.GetSelectHandle(), EVFILT_READ, EV_ADD, 0, 0, NULL);
            EV_SET(&changes[1], dirHandle, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, NULL);
            struct kevent event;
            for (;;) {
                int keventResult = kevent(kqueueHandle, changes, 2, &event, 1, NULL);
                if (keventResult < 0) {
                    break;
                }
                if (event.ident == stopSignal.GetSelectHandle()) {
                    break;
                }
                owner->DirectoryMonitorChangeDetected();
            }
        }
    };

    DirectoryMonitor::DirectoryMonitor()
        : _impl(new DirectoryMonitorImpl())
    {
    }


    DirectoryMonitor::DirectoryMonitor(DirectoryMonitor&& other) noexcept
        : _impl(std::move(other._impl))
    {
    }

    DirectoryMonitor::DirectoryMonitor(std::unique_ptr< DirectoryMonitorImpl >&& impl) noexcept
        : _impl(std::move(impl))
    {
    }

    DirectoryMonitor::~DirectoryMonitor() {
        Stop();
    }

    DirectoryMonitor& DirectoryMonitor::operator=(DirectoryMonitor&& other) noexcept {
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
        _impl->dirHandle = open(path.c_str(), O_EVTONLY);
        if (_impl->dirHandle < 0) {
            return false;
        }
        _impl->stopSignal.Clear();
        _impl->owner = owner;
        _impl->kqueueHandle = kqueue();
        if (_impl->kqueueHandle < 0) {
            (void)close(_impl->dirHandle);
            _impl->dirHandle = -1;
            return false;
        }
        _impl->worker = std::thread(&DirectoryMonitorImpl::Run, _impl.get());
        return true;
    }
    
    void DirectoryMonitor::Stop() {
        if (!_impl->worker.joinable()) {
            return;
        }
        _impl->stopSignal.Set();
        _impl->worker.join();
        (void)close(_impl->kqueueHandle);
        _impl->kqueueHandle = -1;
        (void)close(_impl->dirHandle);
        _impl->dirHandle = -1;
    }
    
}
