#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_WIN32_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_WIN32_HPP

/**
 * @file NetworkConnectionWin32.hpp
 *
 * This module declares the Windows implementation of the
 * SystemAbstractions::NetworkConnectionPlatform structure.
 *
 * © 2016-2018 by Richard Walters
 */

#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/NetworkConnection.hpp>

#include <deque>
#include <mutex>
#include <stdint.h>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkConnectionPlatform class.
     */
    struct NetworkConnection::Platform {
        // Properties

        /**
         * This keeps track of whether or not WSAStartup succeeded,
         * because if so we need to call WSACleanup upon teardown.
         */
        bool wsaStarted = false;

        /**
         * This is the operating system handle to the network
         * port bound by this object.
         */
        SOCKET sock = INVALID_SOCKET;

        /**
         * This is the thread which performs all the actual
         * sending and receiving of data over the network.
         */
        std::thread processor;

        /**
         * This is an event used with WSAEventSelect in order
         * for the worker thread to wait for interesting things
         * to happen with the bound network port.
         */
        HANDLE socketEvent = NULL;

        /**
         * This is an event used to wake up the worker thread
         * if a new message to send has been placed in the
         * output queue, or if we want the worker thread to stop.
         */
        HANDLE processorStateChangeEvent = NULL;

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
        std::deque< uint8_t > outputQueue;

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
            SOCKET sock,
            uint32_t boundAddress,
            uint16_t boundPort,
            uint32_t peerAddress,
            uint16_t peerPort
        );
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_WIN32_HPP */
