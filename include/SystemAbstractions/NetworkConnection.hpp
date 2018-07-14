#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP

/**
 * @file NetworkConnection.hpp
 *
 * This module declares the SystemAbstractions::NetworkConnection class.
 *
 * © 2016-2018 by Richard Walters
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
             * This is callback issued whenever more data is received from
             * the peer of the connection.
             *
             * @param[in] message
             *     This contains the data received from
             *     the peer of the connection.
             */
            virtual void NetworkConnectionMessageReceived(const std::vector< uint8_t >& message) {}

            /**
             * This is callback issued whenever the connection is broken.
             */
            virtual void NetworkConnectionBroken() {}
        };

        // Lifecycle Management
    public:
        ~NetworkConnection() noexcept;
        NetworkConnection(const NetworkConnection&) noexcept = delete;
        NetworkConnection(NetworkConnection&& other) noexcept = delete;
        NetworkConnection& operator=(const NetworkConnection&) noexcept = delete;
        NetworkConnection& operator=(NetworkConnection&& other) noexcept = delete;

        // Public methods
    public:
        /**
         * This is an instance constructor.
         */
        NetworkConnection();

        /**
         * This method forms a new subscription to diagnostic
         * messages published by the sender.
         *
         * @param[in] delegate
         *     This is the function to call to deliver messages
         *     to this subscriber.
         *
         * @param[in] minLevel
         *     This is the minimum level of message that this subscriber
         *     desires to receive.
         *
         * @return
         *     A token representing the subscription is returned.
         *     This may be passed to UnsubscribeFromDiagnostics
         *     in order to terminate the subscription.
         */
        DiagnosticsSender::SubscriptionToken SubscribeToDiagnostics(DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel = 0);

        /**
         * This method terminates a subscription previously formed
         * by calling the SubscribeToDiagnostics method.
         *
         * @param[in] subscriptionToken
         *     This is the token returned from SubscribeToDiagnostics
         *     when the subscription was formed.
         */
        void UnsubscribeFromDiagnostics(DiagnosticsSender::SubscriptionToken subscriptionToken);

        /**
         * This method attempts to establish a connection to a remote peer.
         *
         * @param[in] peerAddress
         *     This is the IPv4 address of the peer.
         *
         * @param[in] peerPort
         *     This is the port number of the peer.
         *
         * @return
         *     An indication of whether or not the connection was successfully
         *     established is returned.
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
         * This method returns the IPv4 address of the peer, if there
         * is a connection established.
         *
         * @return
         *     The IPv4 address of the peer, if there is a connection
         *     established, is returned.
         */
        uint32_t GetPeerAddress() const;

        /**
         * This method returns the port number of the peer, if there
         * is a connection established.
         *
         * @return
         *     The port number of the peer, if there is a connection
         *     established, is returned.
         */
        uint16_t GetPeerPort() const;

        /**
         * This method returns an indication of whether or not there
         * is a connection currently established with a peer.
         *
         * @return
         *     An indication of whether or not there
         *     is a connection currently established with a peer
         *     is returned.
         */
        bool IsConnected() const;

        /**
         * This method appends the given data to the queue of data
         * currently being sent to the peer.  The actual sending
         * is performed by the processor worker thread.
         *
         * @param[in] message
         *     This holds the data to be appended to the send queue.
         */
        void SendMessage(const std::vector< uint8_t >& message);

        /**
         * This method immediately breaks the connection to the peer.
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
