#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP

/**
 * @file NetworkConnection.hpp
 *
 * This module declares the SystemAbstractions::NetworkConnection class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "DiagnosticsSender.hpp"

#include <memory>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class handles the exchanging of messages between its owner and a
     * remote object, over a network.
     */
    class NetworkConnection {
        // Types
    public:
        /**
         * This is the type of structure that contains the private
         * properties of the instance.  It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This is the type of structure that contains the platform-specific
         * private properties of the instance.  It is defined in the
         * platform-specific part of the implementation and declared here to
         * ensure that it is scoped inside the class.
         */
        struct Platform;

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
         * This is the instance destructor.
         */
        ~NetworkConnection();

        /**
         * @todo Needs documentation
         */
        DiagnosticsSender::SubscriptionToken SubscribeToDiagnostics(DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel = 0);

        /**
         * @todo Needs documentation
         */
        void UnsubscribeFromDiagnostics(DiagnosticsSender::SubscriptionToken subscriptionToken);

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

        // Private properties
    private:
        /**
         * This contains the private properties of the instance.
         */
        std::unique_ptr< struct Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP */
