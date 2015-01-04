/**
 * @file OSFile.cpp
 *
 * This module contains the platform-independent part of the
 * implementation of the Files::OSFile class.
 *
 * Copyright (c) 2013-2014 by Richard Walters
 */

#include "OSFile.h"

namespace Files {

    std::string OSFile::GetPath() const {
        return _path;
    }

    size_t OSFile::Peek(Buffer& buffer, size_t numBytes, size_t offset) const {
        if (numBytes == 0) {
            numBytes = buffer.size();
        }
        return Peek(&buffer[offset], numBytes);
    }

    size_t OSFile::Read(Buffer& buffer, size_t numBytes, size_t offset) {
        if (numBytes == 0) {
            numBytes = buffer.size() - offset;
        }
        if (numBytes == 0) {
            return 0;
        }
        return Read(&buffer[offset], numBytes);
    }

    size_t OSFile::Write(const Buffer& buffer, size_t numBytes, size_t offset) {
        if (numBytes == 0) {
            numBytes = buffer.size() - offset;
        }
        if (numBytes == 0) {
            return 0;
        }
        return Write(&buffer[offset], numBytes);
    }

}
