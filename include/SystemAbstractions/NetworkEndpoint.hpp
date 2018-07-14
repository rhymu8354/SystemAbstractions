#ifndef PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP
#define PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP

/**
 * @file NetworkEndpoint.hpp
 *
 * This module declares the SystemAbstractions::NetworkEndpoint class.
 *
 * © 2016-2018 by Richard Walters
 */

#include "DiagnosticsSender.hpp"
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
         * This is the type of callback function to be called whenever
         * a new client connects to the network endpoint.
         *
         * @param[in] newConnection
         *     This represents the connection to the new client.
         */
        typedef std::function< void(std::shared_ptr< NetworkConnection > newConnection) > NewConnectionDelegate;

        /**
         * This is the type of callback function to be called whenever
         * a new datagram-oriented message is received by the network endpoint.
         *
         * @param[in] address
         *     This is the IPv4 address of the client who sent the message.
         *
         * @param[in] port
         *     This is the port number of the client who sent the message.
         *
         * @param[in] body
         *     This is the contents of the datagram sent by the client.
         */
        typedef std::function<
            void(
                uint32_t address,
                uint16_t port,
                const std::vector< uint8_t >& body
            )
        > PacketReceivedDelegate;

        /**
         * These are the different sets of behavior that can be
         * configured for a network endpoint.
         */
        enum class Mode {
            /**
             * In this mode, the network endpoint is not connection-oriented,
             * and only sends and receives unicast packets.
             */
            Datagram,

            /**
             * In this mode, the network endpoint listens for connections
             * from clients, and produces NetworkConnection objects for
             * each client connection established.
             */
            Connection,

            /**
             * In this mode, the network endpoint is set up to only
             * send multicast or broadcast packets.
             */
            MulticastSend,

            /**
             * In this mode, the network endpoint is set up to only
             * receive multicast or broadcast packets.
             */
            MulticastReceive,
        };

        // Lifecycle Management
    public:
        ~NetworkEndpoint() noexcept;
        NetworkEndpoint(const NetworkEndpoint&) noexcept = delete;
        NetworkEndpoint(NetworkEndpoint&& other) noexcept;
        NetworkEndpoint& operator=(const NetworkEndpoint&) noexcept = delete;
        NetworkEndpoint& operator=(NetworkEndpoint&& other) noexcept;

        // Public methods
    public:
        /**
         * This is the instance constructor.
         */
        NetworkEndpoint();

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
        DiagnosticsSender::SubscriptionToken SubscribeToDiagnostics(
            DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        );

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
         * This method starts message or connection processing on the endpoint,
         * depending on the given mode.
         *
         * @param[in] newConnectionDelegate
         *     This is the callback function to be called whenever
         *     a new client connects to the network endpoint.
         *
         * @param[in] packetReceivedDelegate
         *     This is the callback function to be called whenever
         *     a new datagram-oriented message is received by
         *     the network endpoint.
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
            NewConnectionDelegate newConnectionDelegate,
            PacketReceivedDelegate packetReceivedDelegate,
            Mode mode,
            uint32_t localAddress,
            uint32_t groupAddress,
            uint16_t port
        );

        /**
         * This method returns the network port that the endpoint
         * has bound for its use.
         *
         * @return
         *     The network port that the endpoint
         *     has bound for its use is returned.
         */
        uint16_t GetBoundPort() const;

        /**
         * This method is used when the network endpoint is configured
         * to send datagram messages (not connection-oriented).
         * It is called to send a message to one or more recipients.
         *
         * @param[in] address
         *     This is the IPv4 address of the recipient of the message.
         *
         * @param[in] port
         *     This is the port of the recipient of the message.
         *
         * @param[in] body
         *     This is the desired payload of the message.
         */
        void SendPacket(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        );

        /**
         * This method is the opposite of the Open method.  It stops
         * any and all network activity associated with the endpoint,
         * and releases any network resources previously acquired.
         */
        void Close();

        /**
         * This is a helper free function which determines the IPv4
         * addresses of all active network interfaces on the local host.
         *
         * @return
         *     The IPv4 addresses of all active network interfaces on
         *     the local host are returned.
         */
        static std::vector< uint32_t > GetInterfaceAddresses();

        // Private properties
    private:
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
         * This contains the private properties of the instance.
         */
        std::unique_ptr< struct Impl > impl_;
    };

}

#endif /* PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP */
