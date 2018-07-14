#ifndef SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_WIN32_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_WIN32_HPP

/**
 * @file NetworkEndpointWin32.hpp
 *
 * This module declares the Windows implementation of the
 * SystemAbstractions::NetworkEndpointPlatform structure.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <SystemAbstractions/NetworkEndpoint.hpp>

#include <list>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <vector>

namespace SystemAbstractions {

    /**
     * This is the Win32-specific state for the NetworkEndpoint class.
     */
    struct NetworkEndpoint::Platform {
        /**
         * @todo Needs documentation
         */
        struct Packet {
            uint32_t address;
            uint16_t port;
            std::vector< uint8_t > body;
        };

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
        std::list< Packet > outputQueue;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_WIN32_HPP */
