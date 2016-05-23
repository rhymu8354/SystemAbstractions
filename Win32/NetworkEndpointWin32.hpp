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

#include "../NetworkEndpoint.hpp"

#include <stdint.h>
#include <thread>
#include <WinSock2.h>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkEndpointPlatform class.
     */
    struct NetworkEndpointPlatform {
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
        std::thread listener;

        /**
         * @todo Needs documentation
         */
        HANDLE incomingClientEvent = NULL;

        /**
         * @todo Needs documentation
         */
        HANDLE listenerStopEvent = NULL;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_WIN32_HPP */
