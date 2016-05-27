#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP

/**
 * @file NetworkConnection.hpp
 *
 * This module declares the SystemAbstractions::NetworkConnection class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "DiagnosticsReceiver.hpp"

#include <memory>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class handles the exchanging of messages between its owner and a
     * remote object, over a network.
     */
    class NetworkConnection {
        // Custom types
    public:
        /**
         * This is implemented by the owner of the connection and
         * used to deliver callbacks.
         */
        class Owner {
        public:
            /**
             * @todo Needs documentation
             */
            virtual void NetworkConnectionMessageReceived(const std::vector< uint8_t >& message) {}

            /**
             * @todo Needs documentation
             */
            virtual void NetworkConnectionBroken() {}
        };

        // Public methods
    public:
        /**
         * This is an instance constructor.
         */
        NetworkConnection();

        /**
         * This is an instance constructor.
         *
         * @param impl
         *     This is an existing set of properties to encapsulate
         *     into a new network connection object.
         */
        NetworkConnection(std::unique_ptr< struct NetworkConnectionImpl >&& impl);

        /**
         * This is the instance move constructor.
         */
        NetworkConnection(NetworkConnection&& other);

        /**
         * This is the instance destructor.
         */
        ~NetworkConnection();

        /**
         * This is the move assignment operator.
         */
        NetworkConnection& operator=(NetworkConnection&& other);

        /**
         * @todo Needs documentation
         */
        void SubscribeToDiagnostics(DiagnosticsReceiver* subscriber, size_t minLevel = 0);

        /**
         * @todo Needs documentation
         */
        void UnsubscribeFromDiagnostics(DiagnosticsReceiver* subscriber);

        /**
         * @todo Needs documentation
         */
        bool Connect(uint32_t peerAddress, uint16_t peerPort);

        /**
         * This method starts message processing on the connection,
         * listening for incoming messages and sending outgoing messages.
         *
         * @param[in] owner
         *     This is a reference to the owner which should receive
         *     any callbacks from the object.
         *
         * @return
         *     An indication of whether or not the method was
         *     successful is returned.
         */
        bool Process(Owner* owner);

        /**
         * @todo Needs documentation
         */
        uint32_t GetPeerAddress() const;

        /**
         * @todo Needs documentation
         */
        uint16_t GetPeerPort() const;

        /**
         * @todo Needs documentation
         */
        bool IsConnected() const;

        /**
         * @todo Needs documentation
         */
        void SendMessage(const std::vector< uint8_t >& message);

        /**
         * @todo Needs documentation
         */
        void Close();

        // Disable copy constructor and assignment operator.
        NetworkConnection(const NetworkConnection&) = delete;
        NetworkConnection& operator=(const NetworkConnection&) = delete;

        // Private properties
    private:
        std::unique_ptr< struct NetworkConnectionImpl > _impl;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP */
