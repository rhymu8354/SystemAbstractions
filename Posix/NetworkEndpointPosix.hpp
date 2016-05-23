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

#include "../NetworkEndpoint.hpp"
#include "PipeSignal.hpp"

#include <stdint.h>
#include <thread>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkEndpointPlatform class.
     */
    struct NetworkEndpointPlatform {
        /**
         * @todo Needs documentation
         */
        int sock = -1;

        /**
         * @todo Needs documentation
         */
        std::thread listener;

        /**
         * @todo Needs documentation
         */
        PipeSignal listenerStopSignal;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_POSIX_HPP */
