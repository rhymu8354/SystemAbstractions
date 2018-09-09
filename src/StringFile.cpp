/**
 * @file StringFile.cpp
 *
 * This module contains the implementation of the StringFile class.
 *
 * Copyright © 2013-2018 by Richard Walters
 */

#include <SystemAbstractions/StringFile.hpp>

#include <algorithm>
#include <deque>
#include <string>

namespace SystemAbstractions {

    /**
     * This contains the private properties of a StringFile instance.
     */
    struct StringFile::Impl {
        /**
         * This is the contents of the file.
         */
        std::deque< uint8_t > value;

        /**
         * This is the current position in the file.
         */
        size_t position = 0;
    };

    StringFile::StringFile(std::string initialValue)
        : impl_(new Impl())
    {
        impl_->value.assign(initialValue.begin(), initialValue.end());
    }

    StringFile::StringFile(std::vector< uint8_t > initialValue)
        : impl_(new Impl())
    {
        impl_->value.assign(initialValue.begin(), initialValue.end());
    }

    StringFile::~StringFile() noexcept = default;
    StringFile::StringFile(const StringFile& other)
        : impl_(new Impl())
    {
        *impl_ = *other.impl_;
    }
    StringFile::StringFile(StringFile&&) noexcept = default;
    StringFile& StringFile::operator=(const StringFile& other) {
        if (&other != this) {
            *impl_ = *other.impl_;
        }
        return *this;
    }
    StringFile& StringFile::operator=(StringFile&&) noexcept = default;

    StringFile::operator std::string() const {
        return std::string(impl_->value.begin(), impl_->value.end());
    }

    StringFile::operator std::vector< uint8_t >() const {
        return std::vector< uint8_t >(impl_->value.begin(), impl_->value.end());
    }

    StringFile& StringFile::operator=(const std::string &b) {
        impl_->value.assign(b.begin(), b.end());
        impl_->position = 0;
        return *this;
    }

    StringFile& StringFile::operator=(const std::vector< uint8_t > &b) {
        impl_->value.assign(b.begin(), b.end());
        impl_->position = 0;
        return *this;
    }

    void StringFile::Remove(size_t numBytes) {
        impl_->value.erase(impl_->value.begin(), impl_->value.begin() + std::min(numBytes, impl_->value.size()));
        impl_->position = std::max(numBytes, impl_->position) - numBytes;
    }

    uint64_t StringFile::GetSize() const {
        return (uint64_t)impl_->value.size();
    }

    bool StringFile::SetSize(uint64_t size) {
        impl_->value.resize((size_t)size);
        return true;
    }

    uint64_t StringFile::GetPosition() const {
        return (uint64_t)impl_->position;
    }

    void StringFile::SetPosition(uint64_t position) {
        impl_->position = (size_t)position;
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
        const size_t amountCopied = std::min(numBytes, impl_->value.size() - std::min(impl_->value.size(), impl_->position));
        for (size_t i = 0; i < amountCopied; ++i) {
            ((uint8_t*)buffer)[i] = impl_->value[impl_->position + i];
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
        const size_t amountCopied = std::min(numBytes, impl_->value.size() - std::min(impl_->value.size(), impl_->position));
        for (size_t i = 0; i < amountCopied; ++i) {
            ((uint8_t*)buffer)[i] = impl_->value[impl_->position + i];
        }
        impl_->position += amountCopied;
        return amountCopied;
    }

    size_t StringFile::Write(const Buffer& buffer, size_t numBytes, size_t offset) {
        if (numBytes == 0) {
            numBytes = buffer.size();
        }
        if (numBytes == 0) {
            return 0;
        }
        if (impl_->position + numBytes > impl_->value.size()) {
            impl_->value.resize(impl_->position + numBytes);
        }
        for (size_t i = 0; i < numBytes; ++i) {
            impl_->value[impl_->position + i] = buffer[offset + i];
        }
        impl_->position += numBytes;
        return numBytes;
    }

    size_t StringFile::Write(const void* buffer, size_t numBytes) {
        if (numBytes == 0) {
            return 0;
        }
        if (impl_->position + numBytes > impl_->value.size()) {
            impl_->value.resize(impl_->position + numBytes);
        }
        for (size_t i = 0; i < numBytes; ++i) {
            impl_->value[impl_->position + i] = ((const uint8_t*)buffer)[i];
        }
        impl_->position += numBytes;
        return numBytes;
    }

    std::shared_ptr< IFile > StringFile::Clone() {
        auto clone = std::make_shared< StringFile >();
        *clone->impl_ = *impl_;
        return clone;
    }

}
