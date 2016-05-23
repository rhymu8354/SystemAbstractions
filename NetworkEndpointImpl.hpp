#ifndef SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP

/**
 * @file NetworkEndpointImpl.hpp
 *
 * This module declares the SystemAbstractions::NetworkEndpointImpl
 * structure.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "DiagnosticsSender.hpp"
#include "NetworkEndpoint.hpp"

#include <memory>
#include <stdint.h>

namespace SystemAbstractions {

    /**
     * @todo Needs documentation
     */
    struct NetworkEndpointImpl {
        /**
         * @todo Needs documentation
         */
        std::unique_ptr< struct NetworkEndpointPlatform > platform;

        /**
         * @todo Needs documentation
         */
        NetworkEndpoint::Owner* owner = nullptr;

        /**
         * @todo Needs documentation
         */
        uint16_t port = 0;

        /**
         * @todo Needs documentation
         */
        DiagnosticsSender diagnosticsSender;

        /**
         * This is the instance constructor.
         */
        NetworkEndpointImpl();

        /**
         * This is the instance destructor.
         */
        ~NetworkEndpointImpl();

        /**
         * @todo Needs documentation
         */
        bool Listen();

        /**
         * @todo Needs documentation
         */
        void Listener();

        /**
         * @todo Needs documentation
         */
        void Close();
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP */
