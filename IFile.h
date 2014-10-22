#ifndef FILES_I_FILE_H
#define FILES_I_FILE_H
#ifndef __cplusplus
#error This is a C++ only header file
#endif

/**
 * @file IFile.h
 *
 * This module declares the Files::IFile interface.
 *
 * Copyright (c) 2013-2014 by Richard Walters
 */

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace Files {

    /**
     * This is the interface to an object holding a mutable array of
     * bytes and a movable pointer into it.
     */
    class IFile {
        // Custom types
    public:
        typedef std::vector< uint8_t > Buffer;

        // Methods
    public:
        virtual ~IFile() {}

        /**
         * This method returns the size of the file in bytes.
         *
         * @return
         *     The size of the file in bytes is returned.
         */
        virtual uint64_t GetSize() const = 0;

        /**
         * This method extends or truncates the file so that it
         * its size becomes the given number of bytes.
         *
         * @param[in] size
         *     This is the desired file size in bytes.
         *
         * @return
         *     A flag indicating whether or not the method succeeeded
         *     is returned.
         */
        virtual bool SetSize(uint64_t size) = 0;

        /**
         * This method returns the current position in the file in bytes.
         *
         * @return
         *     The current position in the file in bytes is returned.
         */
        virtual uint64_t GetPosition() const = 0;

        /**
         * This method sets the current position in the file in bytes.
         *
         * @param[in] position
         *     This is the position in the file in bytes to make current.
         */
        virtual void SetPosition(uint64_t position) = 0;

        /**
         * This method reads a region of the file without advancing the
         * current position in the file.
         *
         * @param[in,out] buffer
         *     This will be modified to contain bytes read from the file.
         *
         * @param[in] numBytes
         *     This is the number of bytes to read from the file.
         *
         * @param[in] offset
         *     This is the byte offset in the buffer where to store the
         *     first byte read from the file.
         *
         * @return
         *     The number of bytes actually read is returned.
         */
        virtual size_t Peek(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) const = 0;

        /**
         * This method reads a region of the file without advancing the
         * current position in the file.
         *
         * @param[out] buffer
         *     This is where to put the bytes read from the file.
         *
         * @param[in] numBytes
         *     This is the number of bytes to read from the file.
         *
         * @return
         *     The number of bytes actually read is returned.
         */
        virtual size_t Peek(void* buffer, size_t numBytes) const = 0;

        /**
         * This method reads a region of the file and advances the
         * current position in the file to be at the byte after the
         * last byte read.
         *
         * @param[in,out] buffer
         *     This will be modified to contain bytes read from the file.
         *
         * @param[in] numBytes
         *     This is the number of bytes to read from the file.
         *
         * @param[in] offset
         *     This is the byte offset in the buffer where to store the
         *     first byte read from the file.
         *
         * @return
         *     The number of bytes actually read is returned.
         */
        virtual size_t Read(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) = 0;

        /**
         * This method reads a region of the file and advances the
         * current position in the file to be at the byte after the
         * last byte read.
         *
         * @param[out] buffer
         *     This is where to put the bytes read from the file.
         *
         * @param[in] numBytes
         *     This is the number of bytes to read from the file.
         *
         * @return
         *     The number of bytes actually read is returned.
         */
        virtual size_t Read(void* buffer, size_t numBytes) = 0;

        /**
         * This method writes a region of the file and advances the
         * current position in the file to be at the byte after the
         * last byte written.
         *
         * @param[in] buffer
         *     This contains the bytes to write to the file.
         *
         * @param[in] numBytes
         *     This is the number of bytes to write to the file.
         *
         * @param[in] offset
         *     This is the byte offset in the buffer where to fetch the
         *     first byte to write to the file.
         *
         * @return
         *     The number of bytes actually written is returned.
         */
        virtual size_t Write(const Buffer& buffer, size_t numBytes = 0, size_t offset = 0) = 0;

        /**
         * This method writes a region of the file and advances the
         * current position in the file to be at the byte after the
         * last byte written.
         *
         * @param[in] buffer
         *     This is where to fetch the bytes to write to the file.
         *
         * @param[in] numBytes
         *     This is the number of bytes to write to the file.
         *
         * @return
         *     The number of bytes actually written is returned.
         */
        virtual size_t Write(const void* buffer, size_t numBytes) = 0;
    };

}

#endif /* FILES_I_FILE_H */
