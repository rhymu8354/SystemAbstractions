#ifndef FILES_STRING_FILE_H
#define FILES_STRING_FILE_H
#ifndef __cplusplus
#error This is a C++ only header file
#endif

/**
 * @file StringFile.h
 *
 * This module declares the StringFile class.
 *
 * Copyright (c) 2013-2015 by Richard Walters
 */

#include "IFile.h"

#include <string>

namespace Files {

    /**
     * This class represents a file stored in a string.
     */
    class StringFile: public IFile {
        // Public methods
    public:
        /**
         * This is the instance constructor.
         *
         * @param[in] initialValue
         *     This is the initial contents of the file.
         */
        StringFile(std::string initialValue = "");

        /**
         * This is the instance destructor.
         */
        ~StringFile();

        /**
         * This is the typecast to std::string operator.
         */
        operator std::string() const;

        /**
         * This is the assignment from std::string operator.
         */
        StringFile& operator =(const std::string &b);

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

        // Private properties
    private:
        /**
         * This is the contents of the file.
         */
        std::string _value;

        /**
         * This is the current position in the file.
         */
        std::string::size_type _position;
    };

}

#endif /* FILES_STRING_FILE_H */
