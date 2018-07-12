/**
 * @file File.cpp
 *
 * This module contains the platform-independent part of the
 * implementation of the SystemAbstractions::File class.
 *
 * Copyright (c) 2013-2016 by Richard Walters
 */

#include "FileImpl.hpp"

#include <SystemAbstractions/File.hpp>

namespace SystemAbstractions {

    std::string File::GetPath() const {
        return impl_->path;
    }

    size_t File::Peek(Buffer& buffer, size_t numBytes, size_t offset) const {
        if (numBytes == 0) {
            numBytes = buffer.size();
        }
        return Peek(&buffer[offset], numBytes);
    }

    size_t File::Read(Buffer& buffer, size_t numBytes, size_t offset) {
        if (numBytes == 0) {
            numBytes = buffer.size() - offset;
        }
        if (numBytes == 0) {
            return 0;
        }
        return Read(&buffer[offset], numBytes);
    }

    size_t File::Write(const Buffer& buffer, size_t numBytes, size_t offset) {
        if (numBytes == 0) {
            numBytes = buffer.size() - offset;
        }
        if (numBytes == 0) {
            return 0;
        }
        return Write(&buffer[offset], numBytes);
    }

    bool File::CreateDirectory(const std::string& directory) {
        std::string directoryWithSeparator(directory);
        if (
            (directoryWithSeparator.length() > 0)
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '\\')
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/')
        ) {
            directoryWithSeparator += '/';
        }
        return File::Impl::CreatePath(directoryWithSeparator);
    }

}
