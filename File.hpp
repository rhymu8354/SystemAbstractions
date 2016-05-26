#ifndef SYSTEM_ABSTRACTIONS_FILE_HPP
#define SYSTEM_ABSTRACTIONS_FILE_HPP

/**
 * @file File.hpp
 *
 * This module declares the SystemAbstractions::File class.
 *
 * Copyright (c) 2013-2016 by Richard Walters
 */

#include "IFile.hpp"

#include <memory>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class represents a file accessed through the
     * native operating system.
     */
    class File: public IFile {
        // Public methods
    public:
        /**
         * This is the instance constructor.
         *
         * @param[in] path
         *     This is the path to the file in the file system.
         */
        File(std::string path);

        /**
         * This is the instance destructor.
         */
        ~File();

        /**
         * This method is used to check if the file exists in the
         * file system.
         *
         * @return
         *     A flag is returned that indicates whether or not the
         *     file exists in the file system.
         */
        bool IsExisting();

        /**
         * This method is used to check if the file is a directory.
         *
         * @return
         *     A flag is returned that indicates whether or not the
         *     file exists in the file system as a directory.
         */
        bool IsDirectory();

        /**
         * This method opens the file for reading, expecting it to
         * already exist.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        bool Open();

        /**
         * This method closes the file, applying any changes made to it.
         */
        void Close();

        /**
         * This method opens the file for reading and writing, creating
         * it if it does not already exist.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        bool Create();

        /**
         * This method destroys the file in the file system.
         */
        void Destroy();

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
        bool Move(const std::string& newPath);

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
        bool Copy(const std::string& destination);

        /**
         * This method returns the time the file was last modified.
         *
         * @return
         *     The time the file was last modified is returned.
         */
        time_t GetLastModifiedTime() const;

        /**
         * This method returns the path of the file.
         *
         * @return
         *     The path of the file is returned.
         */
        std::string GetPath() const;

        /**
         * This method returns the absolute path of the application's
         * executable image.
         *
         * @return
         *     The absolute path of the application's executable image
         *     is returned.
         */
        static std::string GetExeImagePath();

        /**
         * This method returns the directory containing the application's
         * executable image.
         *
         * @return
         *     The directory containing the application's executable image
         *     is returned.
         */
        static std::string GetExeParentDirectory();

        /**
         * This method returns the path to the application resource file
         * with the given name.
         *
         * @param[in] name
         *     This is the name of the application resource file to find.
         *
         * @return
         *     The path to the given application resource file is returned.
         */
        static std::string GetResourceFilePath(const std::string& name);

        /**
         * This method returns the directory containing the application's
         * local per user configuration files.
         *
         * @param[in] nameKey
         *     This is the name of the application.
         *
         * @return
         *     The directory containing the application's local per
         *     user configuration files is returned.
         */
        static std::string GetLocalPerUserConfigDirectory(const std::string& nameKey);

        /**
         * This method returns the directory containing the user's
         * saved game files.
         *
         * @param[in] nameKey
         *     This is a short string identifying the game, to use for
         *     purposes such as naming the directory containg the user's
         *     saved games.
         *
         * @return
         *     The directory containing the user's saved game files
         *     is returned.
         */
        static std::string GetUserSavedGamesDirectory(const std::string& nameKey);

        /**
         * This method lists the contents of a directory.
         *
         * @param[in] directory
         *     This is the directory to list.
         *
         * @param[out] list
         *     This is where to store the list of directory entries.
         */
        static void ListDirectory(const std::string& directory, std::vector< std::string >& list);

        /**
         * This method deletes a directory and all its contents.
         *
         * @param[in] directory
         *     This is the directory to delete.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        static bool DeleteDirectory(const std::string& directory);

        /**
         * This method copies a directory and all its contents.
         *
         * @param[in] existingDirectory
         *     This is the directory to copy.
         *
         * @param[in] newDirectory
         *     This is the destination to which to copy the existing directory.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        static bool CopyDirectory(
            const std::string& existingDirectory,
            const std::string& newDirectory
        );

        // IFile
    public:
        virtual uint64_t GetSize() const override;
        virtual bool SetSize(uint64_t size) override;
        virtual uint64_t GetPosition() const override;
        virtual void SetPosition(uint64_t position) override;
        virtual size_t Peek(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) const override;
        virtual size_t Peek(void* buffer, size_t numBytes) const override;
        virtual size_t Read(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) override;
        virtual size_t Read(void* buffer, size_t numBytes) override;
        virtual size_t Write(const Buffer& buffer, size_t numBytes = 0, size_t offset = 0) override;
        virtual size_t Write(const void* buffer, size_t numBytes) override;

        // Private methods
    private:
        static bool CreatePath(std::string path);

        // Private properties
    private:
        /**
         * This is the path to the file in the file system.
         */
        std::string _path;

        /**
         * This contains any platform-specific state for the object.
         */
        std::unique_ptr< struct FileImpl > _impl;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_FILE_HPP */
