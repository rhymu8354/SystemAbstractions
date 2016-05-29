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
            virtual void NetworkEndpointNewConnection(NetworkConnection&& newConnection) {}
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
        NetworkEndpoint(NetworkEndpoint&& other);

        /**
         * This is the instance destructor.
         */
        ~NetworkEndpoint();

        /**
         * This is the move assignment operator.
         */
        NetworkEndpoint& operator=(NetworkEndpoint&& other);

        /**
         * @todo Needs documentation
         */
        void SubscribeToDiagnostics(DiagnosticsReceiver* subscriber, size_t minLevel = 0);

        /**
         * @todo Needs documentation
         */
        void UnsubscribeFromDiagnostics(DiagnosticsReceiver* subscriber);

        /**
         * This method starts connection listening on the endpoint,
         * listening for incoming connections.
         *
         * @param[in] owner
         *     This is a reference to the owner which should receive
         *     any callbacks from the object.
         *
         * @param[in] address
         *     This is the address to use on the network for
         *     accepting incoming connections.
         *
         * @param[in] port
         *     This is the port number to use on the network for
         *     accepting incoming connections.
         *
         * @return
         *     An indication of whether or not the method was
         *     successful is returned.
         */
        bool ListenForConnections(
            Owner* owner,
            uint32_t address,
            uint16_t port
        );

        /**
         * @todo Needs documentation
         */
        uint32_t GetAddress() const;

        /**
         * @todo Needs documentation
         */
        uint16_t GetPortNumber() const;

        /**
         * @todo Needs documentation
         */
        void Close();

        // Disable copy constructor and assignment operator.
        NetworkEndpoint(const NetworkEndpoint&) = delete;
        NetworkEndpoint& operator=(const NetworkEndpoint&) = delete;

        // Private properties
    private:
        std::unique_ptr< struct NetworkEndpointImpl > _impl;
    };

}

#endif /* PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP */
