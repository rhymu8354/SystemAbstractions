#ifndef PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP
#define PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP

/**
 * @file NetworkEndpoint.hpp
 *
 * This module declares the SystemAbstractions::NetworkEndpoint class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "DiagnosticsReceiver.hpp"
#include "NetworkConnection.hpp"

#include <memory>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class listens for incoming connections from remote objects,
     * constructing network connection objects for them and passing
     * them along to the owner.
     */
    class NetworkEndpoint {
        // Custom types
    public:
        /**
         * This is implemented by the owner of the endpoint and
         * used to deliver callbacks.
         */
        class Owner {
        public:
            /**
             * @todo Needs documentation
             */
            virtual void NetworkEndpointNewConnection(std::shared_ptr< NetworkConnection > newConnection) {}

            /**
             * @todo Needs documentation
             */
            virtual void NetworkEndpointPacketReceived(
                uint32_t address,
                uint16_t port,
                const std::vector< uint8_t >& body
            ) {}
        };

        /**
         * @todo Needs documentation
         */
        enum class Mode {
            Unset,
            Datagram,
            Connection,
            MulticastSend,
            MulticastReceive,
        };

        // Public methods
    public:
        /**
         * This is the instance constructor.
         */
        NetworkEndpoint();

        /**
         * This is the instance move constructor.
         */
        NetworkEndpoint(NetworkEndpoint&& other) noexcept;

        /**
         * This is the instance destructor.
         */
        ~NetworkEndpoint();

        /**
         * This is the move assignment operator.
         */
        NetworkEndpoint& operator=(NetworkEndpoint&& other) noexcept;

        /**
         * @todo Needs documentation
         */
        void SubscribeToDiagnostics(DiagnosticsReceiver* subscriber, size_t minLevel = 0);

        /**
         * @todo Needs documentation
         */
        void UnsubscribeFromDiagnostics(DiagnosticsReceiver* subscriber);

        /**
         * This method starts message or connection processing on the endpoint,
         * depending on the given mode.
         *
         * @param[in] owner
         *     This is a reference to the owner which should receive
         *     any callbacks from the object.
         *
         * @param[in] mode
         *     This selects the kind of processing to perform with
         *     the endpoint.
         *
         * @param[in] localAddress
         *     This is the address to use on the network for the endpoint.
         *     It is only required for multicast send mode.  It is not
         *     used at all for multicast receive mode, since in this mode
         *     the socket requests membership in the multicast group on
         *     all interfaces.  For datagram and connection modes, if an
         *     address is specified, it limits the traffic to a single
         *     interface.
         *
         * @param[in] groupAddress
         *     This is the address to select for multicasting, if a multicast
         *     mode is selected.
         *
         * @param[in] port
         *     This is the port number to use on the network.  For multicast
         *     modes, it is required and is the multicast port number.
         *     For datagram and connection modes, it is optional, and if set,
         *     specifies the local port number to bind; otherwise an
         *     arbitrary ephemeral port is bound.
         *
         * @return
         *     An indication of whether or not the method was
         *     successful is returned.
         */
        bool Open(
            Owner* owner,
            Mode mode,
            uint32_t localAddress,
            uint32_t groupAddress,
            uint16_t port
        );

        /**
         * @todo Needs documentation
         */
        uint16_t GetBoundPort() const;

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
        void Close();

        /**
         * @todo Needs documentation
         */
        static std::vector< uint32_t > GetInterfaceAddresses();

        // Disable copy constructor and assignment operator.
        NetworkEndpoint(const NetworkEndpoint&) = delete;
        NetworkEndpoint& operator=(const NetworkEndpoint&) = delete;

        // Private properties
    private:
        std::unique_ptr< struct NetworkEndpointImpl > _impl;
    };

}

#endif /* PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP */
