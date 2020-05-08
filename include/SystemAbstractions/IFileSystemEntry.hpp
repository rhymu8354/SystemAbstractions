#pragma once

/**
 * @file IFileSystemEntry.hpp
 *
 * This module declares an abstract interface for an object representing
 * an entry in a file system.
 */

#include "IFile.hpp"

#include <string>
#include <time.h>

namespace SystemAbstractions {

    /**
     * This class represents an entry in a file system.
     */
    class IFileSystemEntry : public IFile {
    public:
        /**
         * This method is used to check if the file exists in the
         * file system.
         *
         * @return
         *     A flag is returned that indicates whether or not the
         *     file exists in the file system.
         */
        virtual bool IsExisting() = 0;

        /**
         * This method is used to check if the file is a directory.
         *
         * @return
         *     A flag is returned that indicates whether or not the
         *     file exists in the file system as a directory.
         */
        virtual bool IsDirectory() = 0;

        /**
         * This method opens the file for reading, expecting it to
         * already exist.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        virtual bool OpenReadOnly() = 0;

        /**
         * This method closes the file, applying any changes made to it.
         */
        virtual void Close() = 0;

        /**
         * This method opens the file for reading and writing, creating
         * it if it does not already exist.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        virtual bool OpenReadWrite() = 0;

        /**
         * This method destroys the file in the file system.
         */
        virtual void Destroy() = 0;

        /**
         * This method moves the file to a new path in the file system.
         *
         * @param[in] newPath
         *     This is the new path to which to move the file.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        virtual bool Move(const std::string& newPath) = 0;

        /**
         * This method copies the file to another location in the file system.
         *
         * @param[in] destination
         *     This is the file name and path to create as a copy of the file.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        virtual bool Copy(const std::string& destination) = 0;

        /**
         * This method returns the time the file was last modified.
         *
         * @return
         *     The time the file was last modified is returned.
         */
        virtual time_t GetLastModifiedTime() const = 0;

        /**
         * This method returns the path of the file.
         *
         * @return
         *     The path of the file is returned.
         */
        virtual std::string GetPath() const = 0;
    };

}
