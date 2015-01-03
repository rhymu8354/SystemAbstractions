#ifndef FILES_O_S_FILE_H
#define FILES_O_S_FILE_H
#ifndef __cplusplus
#error This is a C++ only header file
#endif

/**
 * @file OSFile.h
 *
 * This module declares the Files::OSFile class.
 *
 * Copyright (c) 2013-2014 by Richard Walters
 */

#include "IFile.h"

#include <memory>
#include <string>
#include <vector>

namespace Files {

    /**
     * This class represents a file accessed through the
     * native operating system.
     */
    class OSFile: public IFile {
        // Public methods
    public:
        /**
         * This is the instance constructor.
         *
         * @param[in] path
         *     This is the path to the file in the file system.
         */
        OSFile(std::string path);

        /**
         * This is the instance destructor.
         */
        ~OSFile();

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
         * This method opens the file, expecting it to already exist.
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
         * This method opens the file, creating it or destroying its
         * previous contents, if any.
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
         * This method returns the directory containing the application's
         * executable image.
         *
         * @return
         *     The directory containing the application's executable image
         *     is returned.
         */
        static std::string GetExeDirectory();

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
         */
        static void DeleteDirectory(const std::string& directory);

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
        bool CreatePath(std::string path);

        // Private properties
    private:
        /**
         * This is the path to the file in the file system.
         */
        std::string _path;

        /**
         * This contains any platform-specific state for the object.
         */
        std::unique_ptr< struct OSFileImpl > _impl;
    };

}

#endif /* FILES_O_S_FILE_H */
