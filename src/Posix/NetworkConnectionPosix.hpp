#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_POSIX_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_POSIX_HPP

/**
 * @file NetworkConnectionPosix.hpp
 *
 * This module declares the POSIX implementation of the
 * SystemAbstractions::NetworkConnectionPlatform structure.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../DataQueue.hpp"
#include "PipeSignal.hpp"

#include <deque>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <thread>

namespace SystemAbstractions {

    /**
     * This structure contains the platform-specific properties of the
     * NetworkConnection class.
     */
    struct NetworkConnection::Platform {
        // Properties

        /**
         * This is the operating system handle to the network
         * port bound by this object.
         */
        int sock = -1;

        /**
         * This flag indicates whether or not the peer of
         * the connection has signaled a graceful close.
         */
        bool peerClosed = false;

        /**
         * This flag indicates whether or not the connection is
         * in the process of being gracefully closed.
         */
        bool closing = false;

        /**
         * This flag indicates whether or not the socket has
         * been shut down (FD_CLOSE indication sent).
         */
        bool shutdownSent = false;

        /**
         * This is the thread which performs all the actual
         * sending and receiving of data over the network.
         */
        std::thread processor;

        /**
         * This is used to wake up the worker thread
         * if a new message to send has been placed in the
         * output queue, or if we want the worker thread to stop.
         */
        PipeSignal processorStateChangeSignal;

        /**
         * This flag indicates whether or not the worker thread
         * should stop.
         */
        bool processorStop = false;

        /**
         * This is used to synchronize access to the object.
         */
        std::recursive_mutex processingMutex;

        /**
         * This temporarily holds data to be sent across the network
         * by the worker thread.  It is filled by the SendMessage method.
         */
        DataQueue outputQueue;

        // Methods

        /**
         * This is a factory method for creating a new NetworkConnection
         * object out of an already established connection.
         *
         * @param[in] sock
         *     This is the network socket for the established connection.
         *
         * @param[in] boundAddress
         *     This is the IPv4 address of the network interface
         *     bound for the established connection.
         *
         * @param[in] boundPort
         *     This is the port number bound for the established connection.
         *
         * @param[in] peerAddress
         *     This is the IPv4 address of the remote peer of the connection.
         *
         * @param[in] peerPort
         *     This is the port number remote peer of the connection.
         */
        static std::shared_ptr< NetworkConnection > MakeConnectionFromExistingSocket(
            int sock,
            uint32_t boundAddress,
            uint16_t boundPort,
            uint32_t peerAddress,
            uint16_t peerPort
        );

        /**
         * This helper method is called from various places to standardize
         * what the class does when it wants to immediately close
         * the connection.
         */
        void CloseImmediately();
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_POSIX_HPP */
