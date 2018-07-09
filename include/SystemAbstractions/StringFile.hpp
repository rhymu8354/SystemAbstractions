#ifndef SYSTEM_ABSTRACTIONS_STRING_FILE_HPP
#define SYSTEM_ABSTRACTIONS_STRING_FILE_HPP

/**
 * @file StringFile.hpp
 *
 * This module declares the SystemAbstractions::StringFile class.
 *
 * Copyright © 2013-2018 by Richard Walters
 */

#include "IFile.hpp"

#include <deque>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class represents a file stored in a string.
     */
    class StringFile: public IFile {
        // Public methods
    public:
        /**
         * This is an instance constructor.
         *
         * @param[in] initialValue
         *     This is the initial contents of the file.
         */
        StringFile(std::string initialValue = "");

        /**
         * This is an instance constructor.
         *
         * @param[in] initialValue
         *     This is the initial contents of the file.
         */
        StringFile(std::vector< uint8_t > initialValue);

        /**
         * This is the instance destructor.
         */
        ~StringFile();

        /**
         * This is the typecast to std::string operator.
         */
        operator std::string() const;

        /**
         * This is the typecast to std::vector< uint8_t > operator.
         */
        operator std::vector< uint8_t >() const;

        /**
         * This is the assignment from std::string operator.
         */
        StringFile& operator =(const std::string &b);

        /**
         * This is the assignment from std::vector< uint8_t > operator.
         */
        StringFile& operator =(const std::vector< uint8_t > &b);

        /**
         * This method removes the given number of bytes from the front
         * of the string, and moves the file pointer back to either
         * the front of the file, or back the same number of bytes removed,
         * whichever is closer.
         *
         * @param[in] numBytes
         *     This is the number of bytes to remove from the front
         *     of the file.
         */
        void Remove(size_t numBytes);

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
        virtual std::shared_ptr< IFile > Clone() override;

        // Private properties
    private:
        /**
         * This is the contents of the file.
         */
        std::deque< uint8_t > _value;

        /**
         * This is the current position in the file.
         */
        size_t _position;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_STRING_FILE_HPP */
