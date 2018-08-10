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
#include <deque>

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
        // Properties

        /**
         * This is where the actual data is stored in the queue.
         * It's in two levels:
         * 1. Queue of elements
         * 2. Each element holds a buffer and an indication of
         *    how many bytes have already been consumed from it.
         */
        std::deque< Element > elements;

        /**
         * This keeps track of the total number of bytes across
         * all elements of the queue.
         */
        size_t totalBytes = 0;

        // Methods

        /**
         * This method tries to copy, move, and/or remove the given number
         * of bytes from the queue, based on the given mode flags.
         * Fewer bytes may be returned/removed if there are fewer bytes in
         * the queue than requested.
         *
         * @param[in] numBytesRequested
         *     This is the number of bytes to try to remove from the queue.
         *
         * @param[in] returnData
         *     This flag indicates whether or not the data should be
         *     copied or moved to the returned buffer.
         *
         * @param[in] removeData
         *     This flag indicates whether or not the data should be
         *     removed from the queue.
         *
         * @return
         *     The bytes actually copied or moved from the queue are returned.
         */
        auto Dequeue(
            size_t numBytesRequested,
            bool returnData,
            bool removeData
        ) -> Buffer {
            Buffer buffer;
            if (!returnData) {
                buffer.reserve(numBytesRequested);
            }
            auto nextElement = elements.begin();
            auto bytesLeftFromQueue = totalBytes;
            while (
                (numBytesRequested > 0)
                && (bytesLeftFromQueue > 0)
            ) {
                if (
                    (nextElement->consumed == 0)
                    && (nextElement->data.size() == numBytesRequested)
                    && buffer.empty()
                ) {
                    if (returnData) {
                        if (removeData) {
                            buffer = std::move(nextElement->data);
                        } else {
                            buffer = nextElement->data;
                        }
                    }
                    if (removeData) {
                        nextElement = elements.erase(nextElement);
                        totalBytes -= numBytesRequested;
                        bytesLeftFromQueue -= numBytesRequested;
                    }
                    break;
                }
                const auto bytesToConsume = std::min(
                    numBytesRequested,
                    nextElement->data.size() - nextElement->consumed
                );
                if (returnData) {
                    (void)buffer.insert(
                        buffer.end(),
                        nextElement->data.begin() + nextElement->consumed,
                        nextElement->data.begin() + nextElement->consumed + bytesToConsume
                    );
                }
                numBytesRequested -= bytesToConsume;
                bytesLeftFromQueue -= numBytesRequested;
                if (removeData) {
                    nextElement->consumed += bytesToConsume;
                    totalBytes -= bytesToConsume;
                    if (nextElement->consumed >= nextElement->data.size()) {
                        nextElement = elements.erase(nextElement);
                    }
                } else {
                    if (nextElement->consumed + bytesToConsume >= nextElement->data.size()) {
                        ++nextElement;
                    }
                }
            }
            return buffer;
        }
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
        impl_->elements.push_back(std::move(newElement));
    }

    void DataQueue::Enqueue(Buffer&& data) {
        impl_->totalBytes += data.size();
        Element newElement;
        newElement.data = std::move(data);
        impl_->elements.push_back(std::move(newElement));
    }

    auto DataQueue::Dequeue(size_t numBytesRequested) -> Buffer {
        return impl_->Dequeue(numBytesRequested, true, true);
    }

    auto DataQueue::Peek(size_t numBytesRequested) -> Buffer {
        return impl_->Dequeue(numBytesRequested, true, false);
    }

    void DataQueue::Drop(size_t numBytesRequested) {
        impl_->Dequeue(numBytesRequested, false, true);
    }

    size_t DataQueue::GetBuffersQueued() const noexcept {
        return impl_->elements.size();
    }

    size_t DataQueue::GetBytesQueued() const noexcept {
        return impl_->totalBytes;
    }

}
