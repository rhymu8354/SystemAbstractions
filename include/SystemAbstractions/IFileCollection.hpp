#ifndef SYSTEM_ABSTRACTIONS_I_FILE_COLLECTION_HPP
#define SYSTEM_ABSTRACTIONS_I_FILE_COLLECTION_HPP

/**
 * @file IFileCollection.hpp
 *
 * This module declares the SystemAbstractions::IFileCollection interface.
 *
 * Copyright (c) 2013-2016 by Richard Walters
 */

#include <stdint.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This interface represents a collection of files organized by path name
     * and providing a means of getting the size and contents of any file
     * within the collection.
     */
    class IFileCollection {
        // Public Methods
    public:
        virtual ~IFileCollection() {}

        /**
         * This method returns the size in bytes of the given file within
         * the collection.
         *
         * @param[in] fileName
         *     This is the name of the file in the collection.
         *
         * @return
         *     The size in bytes of the given file in the collection
         *     is returned.
         */
        virtual uint64_t Size(const std::string& fileName) = 0;

        /**
         * This method retrieves the contents of the given file within
         * the collection and stores it in a standard vector.
         *
         * @param[in] fileName
         *     This is the name of the file to retrieve from the collection.
         *
         * @param[out] fileContents
         *     This is where to store the contents of the file.
         *
         * @return
         *     An indication of whether or not the file was opened
         *     successfully is returned.
         */
        virtual bool Read(const std::string& fileName, std::vector< uint8_t >& fileContents) = 0;

        /**
         * This method retrieves the contents of the given file within
         * the collection and stores it in a memory buffer provided
         * by the caller.
         *
         * @param[in] fileName
         *     This is the name of the file to retrieve from the collection.
         *
         * @param[in] fileContents
         *     This is the address of a memory buffer where to store the
         *     contents of the file.
         *
         * @param[in] maxFileContents
         *     This is the maximum number of bytes to store in the
         *     provided memory buffer.
         *
         * @return
         *     An indication of whether or not the file was opened
         *     successfully is returned.
         */
        virtual bool Read(const std::string& fileName, void* fileContents, size_t maxFileContents) = 0;

        /**
         * This method lists the contents of a directory of files within
         * the collection.
         *
         * @param[in] directory
         *     This is the directory to list.
         *
         * @param[out] list
         *     This is where to store the list of directory entries.
         */
        virtual void ListDirectory(const std::string& directory, std::vector< std::string >& list) = 0;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_I_FILE_COLLECTION_HPP */
