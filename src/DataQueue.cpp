/**
 * @file DataQueue.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DataQueue class.
 *
 * © 2018 by Richard Walters
 */

#include "DataQueue.hpp"

#include <algorithm>
#include <stddef.h>
#include <queue>

namespace {

    /**
     * This represents one sequential piece of data being
     * held in a DataQueue.
     */
    struct Element {
        /**
         * This holds the actual bytes in the queue element.
         */
        SystemAbstractions::DataQueue::Buffer data;

        /**
         * This is the number of bytes that have already
         * been consumed from this element.
         */
        size_t consumed = 0;
    };

}

namespace SystemAbstractions {

    /**
     * This holds the private properties of the DataQueue class.
     */
    struct DataQueue::Impl {
        /**
         * This is where the actual data is stored in the queue.
         * It's in two levels:
         * 1. Queue of elements
         * 2. Each element holds a buffer and an indication of
         *    how many bytes have already been consumed from it.
         */
        std::queue< Element > elements;

        /**
         * This keeps track of the total number of bytes across
         * all elements of the queue.
         */
        size_t totalBytes = 0;
    };

    DataQueue::~DataQueue() noexcept = default;
    DataQueue::DataQueue(DataQueue&& other) noexcept = default;
    DataQueue& DataQueue::operator=(DataQueue&& other) noexcept = default;

    DataQueue::DataQueue()
        : impl_(new Impl())
    {
    }

    void DataQueue::Enqueue(const Buffer& data) {
        impl_->totalBytes += data.size();
        Element newElement;
        newElement.data = data;
        impl_->elements.push(std::move(newElement));
    }

    void DataQueue::Enqueue(Buffer&& data) {
        impl_->totalBytes += data.size();
        Element newElement;
        newElement.data = std::move(data);
        impl_->elements.push(std::move(newElement));
    }

    auto DataQueue::Dequeue(size_t numBytesRequested) -> Buffer {
        Buffer buffer;
        buffer.reserve(numBytesRequested);
        while (
            (numBytesRequested > 0)
            && (impl_->totalBytes > 0)
        ) {
            auto& nextElement = impl_->elements.front();
            if (
                (nextElement.consumed == 0)
                && (nextElement.data.size() == numBytesRequested)
                && buffer.empty()
            ) {
                buffer = std::move(nextElement.data);
                impl_->elements.pop();
                impl_->totalBytes -= numBytesRequested;
                break;
            }
            const auto bytesToConsume = std::min(
                numBytesRequested,
                nextElement.data.size() - nextElement.consumed
            );
            (void)buffer.insert(
                buffer.end(),
                nextElement.data.begin() + nextElement.consumed,
                nextElement.data.begin() + nextElement.consumed + bytesToConsume
            );
            numBytesRequested -= bytesToConsume;
            impl_->totalBytes -= bytesToConsume;
            nextElement.consumed += bytesToConsume;
            if (nextElement.consumed >= nextElement.data.size()) {
                impl_->elements.pop();
            }
        }
        return buffer;
    }

    size_t DataQueue::GetBuffersQueued() const noexcept {
        return impl_->elements.size();
    }

    size_t DataQueue::GetBytesQueued() const noexcept {
        return impl_->totalBytes;
    }

}
