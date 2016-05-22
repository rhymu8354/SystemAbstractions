#ifndef FILES_DIRECTORY_MONITOR_HPP
#define FILES_DIRECTORY_MONITOR_HPP

/**
 * @file DirectoryMonitor.hpp
 *
 * This module declares the Files::DirectoryMonitor class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <memory>
#include <string>

namespace Files {

    /**
     * This class is used to monitor for changes to a directory
     * in the file system, invoking a callback whenever any change
     * is detected.
     */
    class DirectoryMonitor {
        // Custom Types
    public:
        /**
         * This is implemented by the owner of the object and
         * used to deliver callbacks.
         */
        class Owner {
        public:
            /**
             * @todo Needs documentation
             */
            virtual void DirectoryMonitorChangeDetected() {}
        };

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
        DirectoryMonitor(std::unique_ptr< struct DirectoryMonitorImpl >&& impl);

        /**
         * This is the instance move constructor.
         */
        DirectoryMonitor(DirectoryMonitor&& other);

        /**
         * This is the instance destructor.
         */
        ~DirectoryMonitor();

        /**
         * This is the move assignment operator.
         */
        DirectoryMonitor& operator=(DirectoryMonitor&& other);

        /**
         * @todo Needs documentation
         */
        bool Start(
            Owner* owner,
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

#endif /* FILES_DIRECTORY_MONITOR_HPP */
