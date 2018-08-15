/**
 * @file DirectoryMonitorMach.cpp
 *
 * This module contains the Mach implementation of the
 * SystemAbstractions::DirectoryMonitor class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../Posix/PipeSignal.hpp"

#include <assert.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <thread>
#include <unistd.h>

namespace SystemAbstractions {

    /**
     * This structure contains the private methods and properties of
     * the DirectoryMonitor class.
     */
    struct DirectoryMonitor::Impl {
        // Properties

        /**
         * This is the thread which is waiting for notifications from
         * the operating system about changes to the monitored directory.
         */
        std::thread worker;

        /**
         * This is provided by the owner of the object and
         * called whenever a change is detected to the monitored directory.
         */
        DirectoryMonitor::Callback callback;

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
                callback();
            }
        }
    };

    DirectoryMonitor::~DirectoryMonitor() noexcept {
        Stop();
    }
    DirectoryMonitor::DirectoryMonitor(DirectoryMonitor&& other) noexcept = default;
    DirectoryMonitor& DirectoryMonitor::operator=(DirectoryMonitor&& other) noexcept = default;

    DirectoryMonitor::DirectoryMonitor()
        : impl_(new Impl())
    {
    }

    bool DirectoryMonitor::Start(
        Callback callback,
        const std::string& path
    ) {
        if (impl_ == nullptr) {
            return false;
        }
        Stop();
        if (!impl_->stopSignal.Initialize()) {
            return false;
        }
        impl_->dirHandle = open(path.c_str(), O_EVTONLY);
        if (impl_->dirHandle < 0) {
            return false;
        }
        impl_->stopSignal.Clear();
        impl_->callback = callback;
        impl_->kqueueHandle = kqueue();
        if (impl_->kqueueHandle < 0) {
            (void)close(impl_->dirHandle);
            impl_->dirHandle = -1;
            return false;
        }
        impl_->worker = std::thread(&Impl::Run, impl_.get());
        return true;
    }
    
    void DirectoryMonitor::Stop() {
        if (impl_ == nullptr) {
            return;
        }
        if (!impl_->worker.joinable()) {
            return;
        }
        impl_->stopSignal.Set();
        impl_->worker.join();
        (void)close(impl_->kqueueHandle);
        impl_->kqueueHandle = -1;
        (void)close(impl_->dirHandle);
        impl_->dirHandle = -1;
    }
    
}
