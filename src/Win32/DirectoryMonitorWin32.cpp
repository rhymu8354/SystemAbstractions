/**
 * @file DirectoryMonitorWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::DirectoryMonitor class.
 *
 * © 2016-2018 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>

#include <SystemAbstractions/DirectoryMonitor.hpp>
#include <thread>

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
         * This is the event which is set up to be signaled by
         * the operating system whenever the monitored directory is changed.
         */
        HANDLE changeEvent = INVALID_HANDLE_VALUE;

        /**
         * This is signaled whenever the user stops the directory monitoring.
         */
        HANDLE stopEvent = NULL;

        // Methods

        /**
         * This method is called as the body of the worker thread.  It responds
         * to signals from the user and the operating system.  If the operating
         * system signals a directory change, the thread calls the user's callback.
         */
        void Run() {
            HANDLE handles[2] = { stopEvent, changeEvent };
            while (WaitForMultipleObjects(2, handles, FALSE, INFINITE) != 0) {
                callback();
                if (FindNextChangeNotification(changeEvent) == FALSE) {
                    break;
                }
            }
        }
    };

    DirectoryMonitor::~DirectoryMonitor() {
        if (impl_ != nullptr) {
            Stop();
            if (impl_->stopEvent != NULL) {
                (void)CloseHandle(impl_->stopEvent);
            }
        }
    }

    DirectoryMonitor::DirectoryMonitor(DirectoryMonitor&& other) noexcept
        : impl_(std::move(other.impl_))
    {
    }

    DirectoryMonitor& DirectoryMonitor::operator=(DirectoryMonitor&& other) noexcept {
        if (this != &other) {
            impl_ = std::move(other.impl_);
        }
        return *this;
    }

    DirectoryMonitor::DirectoryMonitor()
        : impl_(new Impl())
    {
    }

    bool DirectoryMonitor::Start(
        Callback callback,
        const std::string& path
    ) {
        Stop();
        if (impl_->stopEvent == NULL) {
            impl_->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (impl_->stopEvent == NULL) {
                return false;
            }
        } else {
            (void)ResetEvent(impl_->stopEvent);
        }
        impl_->callback = callback;
        impl_->changeEvent = FindFirstChangeNotificationA(
            path.c_str(),
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME
            | FILE_NOTIFY_CHANGE_DIR_NAME
            | FILE_NOTIFY_CHANGE_LAST_WRITE
        );
        if (impl_->changeEvent == INVALID_HANDLE_VALUE) {
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
        (void)SetEvent(impl_->stopEvent);
        impl_->worker.join();
        (void)FindCloseChangeNotification(impl_->changeEvent);
    }

}
