#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_WIN32_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_WIN32_HPP

/**
 * @file NetworkConnectionWin32.hpp
 *
 * This module declares the Windows implementation of the
 * SystemAbstractions::NetworkConnectionPlatform structure.
 *
 * Copyright (c) 2016 by Richard Walters
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
         * @todo Needs documentation
         */
        bool wsaStarted = false;

        /**
         * @todo Needs documentation
         */
        SOCKET sock = INVALID_SOCKET;

        /**
         * @todo Needs documentation
         */
        std::thread processor;

        /**
         * @todo Needs documentation
         */
        HANDLE socketEvent = NULL;

        /**
         * @todo Needs documentation
         */
        HANDLE processorStateChangeEvent = NULL;

        /**
         * @todo Needs documentation
         */
        bool processorStop = false;

        /**
         * @todo Needs documentation
         */
        std::recursive_mutex processingMutex;

        /**
         * @todo Needs documentation
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
         * @param[in] address
         *     This is the IPv4 address of the network interface
         *     bound for the established connection.
         *
         * @param[in] port
         *     This is the port number bound for the established connection.
         */
        static std::shared_ptr< NetworkConnection > MakeConnectionFromExistingSocket(
            SOCKET sock,
            uint32_t address,
            uint16_t port
        );
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_WIN32_HPP */
