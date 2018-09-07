#ifndef SYSTEM_ABSTRACTIONS_I_NETWORK_CONNECTION_HPP
#define SYSTEM_ABSTRACTIONS_I_NETWORK_CONNECTION_HPP

/**
 * @file INetworkConnection.hpp
 *
 * This module declares the SystemAbstractions::INetworkConnection interface.
 *
 * © 2016-2018 by Richard Walters
 */

#include "DiagnosticsSender.hpp"

#include <memory>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions {

    /**
     * This is the common interface for objects which handle the exchanging
     * of messages between the user and a remote object, over a network.
     */
    class INetworkConnection {
        // Types
    public:
        /**
         * This is the type of callback issued whenever more data
         * is received from the peer of the connection.
         *
         * @param[in] message
         *     This contains the data received from
         *     the peer of the connection.
         */
        typedef std::function<
            void(const std::vector< uint8_t >& message)
        > MessageReceivedDelegate;

        /**
         * This is the type of callback issued whenever
         * the connection is broken.
         *
         * @param[in] graceful
         *     This indicates whether or not the peer of connection
         *     has closed the connection gracefully (meaning we can
         *     continue to send our data back to the peer).
         */
        typedef std::function< void(bool graceful) > BrokenDelegate;

        // Public methods
    public:
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
         *     A function is returned which may be called
         *     to terminate the subscription.
         */
        virtual DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        ) = 0;

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
        virtual bool Connect(uint32_t peerAddress, uint16_t peerPort) = 0;

        /**
         * This method starts message processing on the connection,
         * listening for incoming messages and sending outgoing messages.
         *
         * @param[in] messageReceivedDelegate
         *     This is the callback issued whenever more data
         *     is received from the peer of the connection.
         *
         * @param[in] brokenDelegate
         *     This is the callback issued whenever
         *     the connection is broken.
         *
         * @return
         *     An indication of whether or not the method was
         *     successful is returned.
         */
        virtual bool Process(
            MessageReceivedDelegate messageReceivedDelegate,
            BrokenDelegate brokenDelegate
        ) = 0;

        /**
         * This method returns the IPv4 address of the peer, if there
         * is a connection established.
         *
         * @return
         *     The IPv4 address of the peer, if there is a connection
         *     established, is returned.
         */
        virtual uint32_t GetPeerAddress() const = 0;

        /**
         * This method returns the port number of the peer, if there
         * is a connection established.
         *
         * @return
         *     The port number of the peer, if there is a connection
         *     established, is returned.
         */
        virtual uint16_t GetPeerPort() const = 0;

        /**
         * This method returns an indication of whether or not there
         * is a connection currently established with a peer.
         *
         * @return
         *     An indication of whether or not there
         *     is a connection currently established with a peer
         *     is returned.
         */
        virtual bool IsConnected() const = 0;

        /**
         * This method returns the IPv4 address that the connection
         * object is using in the current connection.
         *
         * @return
         *     This is the IPv4 address that the connection
         *     object is using in the current connection.
         */
        virtual uint32_t GetBoundAddress() const = 0;

        /**
         * This method returns the port number that the connection
         * object is using in the current connection.
         *
         * @return
         *     This is the port number that the connection
         *     object is using in the current connection.
         */
        virtual uint16_t GetBoundPort() const = 0;

        /**
         * This method appends the given data to the queue of data
         * currently being sent to the peer.  The actual sending
         * is performed by the processor worker thread.
         *
         * @param[in] message
         *     This holds the data to be appended to the send queue.
         */
        virtual void SendMessage(const std::vector< uint8_t >& message) = 0;

        /**
         * This method breaks the connection to the peer.
         *
         * @param[in] clean
         *     This flag indicates whether or not to attempt to complete
         *     any data transmission still in progress, before breaking
         *     the connection.
         */
        virtual void Close(bool clean = false) = 0;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_I_NETWORK_CONNECTION_HPP */
