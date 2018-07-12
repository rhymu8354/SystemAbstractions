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

#include <assert.h>
#include <thread>

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
         * This is provided by the owner of the object and
         * called whenever a change is detected to the monitored directory.
         */
        DirectoryMonitor::Callback callback;

        /**
         * @todo Needs documentation
         */
        HANDLE changeEvent = INVALID_HANDLE_VALUE;

        /**
         * @todo Needs documentation
         */
        HANDLE stopEvent = NULL;

        // Methods

        /**
         * @todo Needs documentation
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
        if (_impl != nullptr) {
            Stop();
            if (_impl->stopEvent != NULL) {
                (void)CloseHandle(_impl->stopEvent);
            }
        }
    }

    DirectoryMonitor& DirectoryMonitor::operator=(DirectoryMonitor&& other) noexcept {
        assert(this != &other);
        _impl = std::move(other._impl);
        return *this;
    }

    bool DirectoryMonitor::Start(
        Callback callback,
        const std::string& path
    ) {
        Stop();
        if (_impl->stopEvent == NULL) {
            _impl->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (_impl->stopEvent == NULL) {
                return false;
            }
        } else {
            (void)ResetEvent(_impl->stopEvent);
        }
        _impl->callback = callback;
        _impl->changeEvent = FindFirstChangeNotificationA(
            path.c_str(),
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME
            | FILE_NOTIFY_CHANGE_DIR_NAME
            | FILE_NOTIFY_CHANGE_LAST_WRITE
        );
        if (_impl->changeEvent == INVALID_HANDLE_VALUE) {
            return false;
        }
        _impl->worker = std::move(std::thread(&DirectoryMonitorImpl::Run, _impl.get()));
        return true;
    }

    void DirectoryMonitor::Stop() {
        if (_impl == nullptr) {
            return;
        }
        if (!_impl->worker.joinable()) {
            return;
        }
        (void)SetEvent(_impl->stopEvent);
        _impl->worker.join();
        (void)FindCloseChangeNotification(_impl->changeEvent);
    }

}
