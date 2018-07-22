#ifndef SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_POSIX_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_POSIX_HPP

/**
 * @file NetworkEndpointPosix.hpp
 *
 * This module declares the POSIX implementation of the
 * PhoenixWays::Core::NetworkEndpointPlatform structure.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "PipeSignal.hpp"

#include <list>
#include <mutex>
#include <stdint.h>
#include <SystemAbstractions/NetworkEndpoint.hpp>
#include <thread>
#include <vector>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkEndpointPlatform class.
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
        std::list< Packet > outputQueue;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_POSIX_HPP */
