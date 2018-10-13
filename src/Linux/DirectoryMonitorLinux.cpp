/**
 * @file DirectoryMonitorLinux.cpp
 *
 * This module contains the Linux implementation of the
 * SystemAbstractions::DirectoryMonitor class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../Posix/PipeSignal.hpp"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <thread>
#include <unistd.h>
#include <vector>

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
                    callback();
                }
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
        impl_->stopSignal.Clear();
        impl_->callback = callback;
        impl_->inotifyQueue = inotify_init();
        if (impl_->inotifyQueue < 0) {
            return false;
        }
        int flags = fcntl(impl_->inotifyQueue, F_GETFL, 0);
        if (flags < 0) {
            (void)close(impl_->inotifyQueue);
            impl_->inotifyQueue = -1;
            return false;
        }
        flags |= O_NONBLOCK;
        if (fcntl(impl_->inotifyQueue, F_SETFL, flags) < 0) {
            (void)close(impl_->inotifyQueue);
            impl_->inotifyQueue = -1;
            return false;
        }
        impl_->inotifyWatch = inotify_add_watch(
            impl_->inotifyQueue,
            path.c_str(),
            IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO
        );
        if (impl_->inotifyWatch < 0) {
            (void)close(impl_->inotifyQueue);
            impl_->inotifyQueue = -1;
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
        impl_->inotifyWatch = -1;
        (void)close(impl_->inotifyQueue);
        impl_->inotifyQueue = -1;
    }

}
