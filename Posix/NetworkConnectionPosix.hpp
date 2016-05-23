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

#include "../DiagnosticsSender.hpp"
#include "PipeSignal.hpp"

#include <deque>
#include <mutex>
#include <stdint.h>
#include <thread>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkConnectionPlatform class.
     */
    struct NetworkConnectionPlatform {
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

        /**
         * @todo Needs documentation
         */
        static bool Bind(
            int& sock,
            uint16_t port,
            DiagnosticsSender& diagnosticsSender
        );
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_POSIX_HPP */
