/**
 * @file StringFile.cpp
 *
 * This module contains the implementation of the StringFile class.
 *
 * Copyright (c) 2013-2016 by Richard Walters
 */

#include "StringFile.hpp"

#include <algorithm>
#include <string>

namespace SystemAbstractions {

    StringFile::StringFile(std::string initialValue)
        : _position(0)
    {
        _value.assign(initialValue.begin(), initialValue.end());
    }

    StringFile::StringFile(std::vector< uint8_t > initialValue)
        : _value(initialValue.begin(), initialValue.end())
        , _position(0)
    {
    }

    StringFile::~StringFile() {
    }

    StringFile::operator std::string() const {
        return std::string(_value.begin(), _value.end());
    }

    StringFile::operator std::vector< uint8_t >() const {
        return std::vector< uint8_t >(_value.begin(), _value.end());
    }

    StringFile& StringFile::operator =(const std::string &b) {
        _value.assign(b.begin(), b.end());
        _position = 0;
        return *this;
    }

    StringFile& StringFile::operator =(const std::vector< uint8_t > &b) {
        _value.assign(b.begin(), b.end());
        _position = 0;
        return *this;
    }

    void StringFile::Remove(size_t numBytes) {
        _value.erase(_value.begin(), _value.begin() + numBytes);
        _position = std::max(numBytes, _position) - numBytes;
    }

    uint64_t StringFile::GetSize() const {
        return (uint64_t)_value.size();
    }

    bool StringFile::SetSize(uint64_t size) {
        _value.resize((size_t)size);
        return true;
    }

    uint64_t StringFile::GetPosition() const {
        return (uint64_t)_position;
    }

    void StringFile::SetPosition(uint64_t position) {
        _position = (size_t)position;
    }

    size_t StringFile::Peek(Buffer& buffer, size_t numBytes, size_t offset) const {
        if (numBytes == 0) {
            numBytes = buffer.size();
        }
        if (numBytes == 0) {
            return 0;
        }
        return Peek(&buffer[offset], numBytes);
    }

    size_t StringFile::Peek(void* buffer, size_t numBytes) const {
        const size_t amountCopied = std::min(numBytes, _value.size() - _position);
        for (size_t i = 0; i < amountCopied; ++i) {
            ((uint8_t*)buffer)[i] = _value[_position + i];
        }
        return amountCopied;
    }

    size_t StringFile::Read(Buffer& buffer, size_t numBytes, size_t offset) {
        if (numBytes == 0) {
            numBytes = buffer.size();
        }
        if (numBytes == 0) {
            return 0;
        }
        return Read(&buffer[offset], numBytes);
    }

    size_t StringFile::Read(void* buffer, size_t numBytes) {
        const size_t amountCopied = std::min(numBytes, _value.size() - _position);
        for (size_t i = 0; i < amountCopied; ++i) {
            ((uint8_t*)buffer)[i] = _value[_position + i];
        }
        _position += amountCopied;
        return amountCopied;
    }

    size_t StringFile::Write(const Buffer& buffer, size_t numBytes, size_t offset) {
        if (numBytes == 0) {
            numBytes = buffer.size();
        }
        if (numBytes == 0) {
            return 0;
        }
        if (_position + numBytes > _value.size()) {
            _value.resize(_position + numBytes);
        }
        for (size_t i = 0; i < numBytes; ++i) {
            _value[_position + i] = buffer[offset + i];
        }
        _position += numBytes;
        return numBytes;
    }

    size_t StringFile::Write(const void* buffer, size_t numBytes) {
        if (numBytes == 0) {
            return 0;
        }
        if (_position + numBytes > _value.size()) {
            _value.resize(_position + numBytes);
        }
        for (size_t i = 0; i < numBytes; ++i) {
            _value[_position + i] = ((const uint8_t*)buffer)[i];
        }
        _position += numBytes;
        return numBytes;
    }

}
