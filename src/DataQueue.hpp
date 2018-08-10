#ifndef SYSTEM_ABSTRACTIONS_DATA_QUEUE_HPP
#define SYSTEM_ABSTRACTIONS_DATA_QUEUE_HPP

/**
 * @file DataQueue.hpp
 *
 * This module declares the SystemAbstractions::DataQueue class.
 *
 * © 2018 by Richard Walters
 */

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class represents a queue of buffers of data,
     * into which arbitrary-sized buffers of data may
     * be enqueued, and arbitrary-sized buffers of data
     * may be dequeued (sizes don't need to match).
     */
    class DataQueue {
        // Types
    public:
        /**
         * This represents a buffer of data either being enqueued
         * or dequeued.
         */
        typedef std::vector< uint8_t > Buffer;

        // Lifecycle management
    public:
        ~DataQueue() noexcept;
        DataQueue(const DataQueue&) = delete;
        DataQueue(DataQueue&& other) noexcept;
        DataQueue& operator=(const DataQueue& other) = delete;
        DataQueue& operator=(DataQueue&& other) noexcept;

        // Public methods
    public:
        /**
         * This is an instance constructor.
         */
        DataQueue();

        /**
         * This method puts a copy of the given data onto the end
         * of the queue.
         *
         * @param[in] data
         *     This is the data to copy and store at the end of the queue.
         */
        void Enqueue(const Buffer& data);

        /**
         * This method moves the given data onto the end of the queue.
         *
         * @param[in] data
         *     This is the data to move to the end of the queue.
         */
        void Enqueue(Buffer&& data);

        /**
         * This method tries to remove the given number of bytes from
         * the queue.  Fewer bytes may be returned if there are fewer
         * bytes in the queue than requested.
         *
         * @param[in] numBytesRequested
         *     This is the number of bytes to try to remove from the queue.
         *
         * @return
         *     The bytes actually removed from the queue are returned.
         */
        Buffer Dequeue(size_t numBytesRequested);

        /**
         * This method tries to copy the given number of bytes from
         * the queue.  Fewer bytes may be returned if there are fewer
         * bytes in the queue than requested.
         *
         * @param[in] numBytesRequested
         *     This is the number of bytes to try to copy from the queue.
         *
         * @return
         *     The bytes actually copy from the queue are returned.
         */
        Buffer Peek(size_t numBytesRequested);

        /**
         * This method tries to remove the given number of bytes from
         * the queue.  Fewer bytes may be removed if there are fewer
         * bytes in the queue than requested.
         *
         * @param[in] numBytesRequested
         *     This is the number of bytes to try to remove from the queue.
         */
        void Drop(size_t numBytesRequested);

        /**
         * This method returns the number of distinct buffers of data
         * currently held in the queue.
         *
         * @note
         *     This method is only really intended to be useful for
         *     unit tests, since the internal organization of buffers
         *     in the queue is subject to change and not intended to be
         *     part of the interface.
         *
         * @return
         *     The number of distinct buffers of data
         *     currently held in the queue is returned.
         */
        size_t GetBuffersQueued() const noexcept;

        /**
         * This method returns the number of bytes of data
         * currently held in the queue.
         *
         * @return
         *     The number of bytes of data
         *     currently held in the queue is returned.
         */
        size_t GetBytesQueued() const noexcept;

        // Private properties
    private:
        /**
         * This contains any platform-specific state for the object.
         */
        struct Impl;

        /**
         * This contains any platform-specific state for the object.
         */
        std::unique_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DATA_QUEUE_HPP */
