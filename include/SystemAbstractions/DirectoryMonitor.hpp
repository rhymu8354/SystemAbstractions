#ifndef SYSTEM_ABSTRACTIONS_DIRECTORY_MONITOR_HPP
#define SYSTEM_ABSTRACTIONS_DIRECTORY_MONITOR_HPP

/**
 * @file DirectoryMonitor.hpp
 *
 * This module declares the SystemAbstractions::DirectoryMonitor class.
 *
 * © 2016-2018 by Richard Walters
 */

#include <functional>
#include <memory>
#include <string>

namespace SystemAbstractions {

    /**
     * This class is used to monitor for changes to a directory
     * in the file system, invoking a callback whenever any change
     * is detected.
     */
    class DirectoryMonitor {
        // Custom Types
    public:
        /**
         * This is provided by the owner of the object and
         * called whenever a change is detected to the monitored directory.
         */
        typedef std::function< void() > Callback;

        // Lifecycle Management
    public:
        ~DirectoryMonitor();
        DirectoryMonitor(const DirectoryMonitor&) = delete;
        DirectoryMonitor(DirectoryMonitor&& other) noexcept;
        DirectoryMonitor& operator=(const DirectoryMonitor&) = delete;
        DirectoryMonitor& operator=(DirectoryMonitor&& other) noexcept;

        // Public Methods
    public:
        /**
         * This is an instance constructor.
         */
        DirectoryMonitor();

        /**
         * @todo Needs documentation
         */
        bool Start(
            Callback callback,
            const std::string& path
        );

        /**
         * @todo Needs documentation
         */
        void Stop();

        // Private properties
    private:
        std::unique_ptr< struct DirectoryMonitorImpl > _impl;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIRECTORY_MONITOR_HPP */
