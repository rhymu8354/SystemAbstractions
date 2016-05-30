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

#include "../DiagnosticsSender.hpp"

#include <deque>
#include <mutex>
#include <stdint.h>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkConnectionPlatform class.
     */
    struct NetworkConnectionPlatform {
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
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_WIN32_HPP */
