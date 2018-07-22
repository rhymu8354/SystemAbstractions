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

#include "PipeSignal.hpp"

#include <deque>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <thread>

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
        int sock = -1;

        /**
         * @todo Needs documentation
         */
        std::thread processor;

        /**
         * @todo Needs documentation
         */
        PipeSignal processorStateChangeSignal;

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
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_POSIX_HPP */
