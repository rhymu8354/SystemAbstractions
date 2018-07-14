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
#include <vector>

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
         * This is the callback function to be called whenever
         * a new client connects to the network endpoint.
         */
        NetworkEndpoint::NewConnectionDelegate newConnectionDelegate;

        /**
         * This is the callback function to be called whenever
         * a new datagram-oriented message is received by
         * the network endpoint.
         */
        NetworkEndpoint::PacketReceivedDelegate packetReceivedDelegate;

        /**
         * @todo Needs documentation
         */
        uint32_t localAddress = 0;

        /**
         * @todo Needs documentation
         */
        uint32_t groupAddress = 0;

        /**
         * @todo Needs documentation
         */
        uint16_t port = 0;

        /**
         * @todo Needs documentation
         */
        NetworkEndpoint::Mode mode = NetworkEndpoint::Mode::Datagram;

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
        bool Open();

        /**
         * @todo Needs documentation
         */
        void Processor();

        /**
         * @todo Needs documentation
         */
        void SendPacket(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        );

        /**
         * @todo Needs documentation
         */
        void Close(bool stopProcessing);

        /**
         * @todo Needs documentation
         */
        static std::vector< uint32_t > GetInterfaceAddresses();
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP */
