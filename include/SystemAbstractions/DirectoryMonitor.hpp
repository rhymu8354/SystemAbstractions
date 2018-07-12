#ifndef SYSTEM_ABSTRACTIONS_DIRECTORY_MONITOR_HPP
#define SYSTEM_ABSTRACTIONS_DIRECTORY_MONITOR_HPP

/**
 * @file DirectoryMonitor.hpp
 *
 * This module declares the SystemAbstractions::DirectoryMonitor class.
 *
 * Copyright (c) 2016 by Richard Walters
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

        // Public Methods
    public:
        /**
         * This is an instance constructor.
         */
        DirectoryMonitor();

        /**
         * This is an instance constructor.
         *
         * @param impl
         *     This is an existing set of properties to encapsulate
         *     into a new network connection object.
         */
        DirectoryMonitor(std::unique_ptr< struct DirectoryMonitorImpl >&& impl) noexcept;

        /**
         * This is the instance move constructor.
         */
        DirectoryMonitor(DirectoryMonitor&& other) noexcept;

        /**
         * This is the instance destructor.
         */
        ~DirectoryMonitor();

        /**
         * This is the move assignment operator.
         */
        DirectoryMonitor& operator=(DirectoryMonitor&& other) noexcept;

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

        // Disable copy constructor and assignment operator.
        DirectoryMonitor(const DirectoryMonitor&) = delete;
        DirectoryMonitor& operator=(const DirectoryMonitor&) = delete;

        // Private properties
    private:
        std::unique_ptr< struct DirectoryMonitorImpl > _impl;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIRECTORY_MONITOR_HPP */
