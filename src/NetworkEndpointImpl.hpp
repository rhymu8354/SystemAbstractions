#ifndef SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP

/**
 * @file NetworkEndpoint::Impl.hpp
 *
 * This module declares the SystemAbstractions::NetworkEndpoint::Impl
 * structure.
 *
 * © 2016-2018 by Richard Walters
 */

#include <memory>
#include <stdint.h>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/NetworkEndpoint.hpp>
#include <vector>

namespace SystemAbstractions {

    /**
     * This contains the private properties of a NetworkEndpoint instance.
     */
    struct NetworkEndpoint::Impl {
        // Properties

        /**
         * This contains any platform-specific private properties
         * of the class.
         */
        std::unique_ptr< Platform > platform;

        /**
         * This is the callback function to be called whenever
         * a new client connects to the network endpoint.
         */
        NewConnectionDelegate newConnectionDelegate;

        /**
         * This is the callback function to be called whenever
         * a new datagram-oriented message is received by
         * the network endpoint.
         */
        PacketReceivedDelegate packetReceivedDelegate;

        /**
         * This is the IPv4 address of the network interface
         * bound by this endpoint.  If zero, then all network
         * interfaces are bound.
         */
        uint32_t localAddress = 0;

        /**
         * This is the multicast or broadcast address to which
         * the network endpoint is bound, if in one of the
         * multicast modes.
         */
        uint32_t groupAddress = 0;

        /**
         * This is the port number bound by this endpoint.
         */
        uint16_t port = 0;

        /**
         * This is the set of behaviors configured for
         * the network endpoint.
         */
        Mode mode = Mode::Datagram;

        /**
         * This is a helper object used to publish diagnostic messages.
         */
        DiagnosticsSender diagnosticsSender;

        // Lifecycle Management

        ~Impl() noexcept;
        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        // Methods

        /**
         * This is the instance constructor.
         */
        Impl();

        /**
         * This method starts message or connection processing on
         * the endpoint, depending on the configured mode.
         */
        bool Open();

        /**
         * This is the main function called for the worker thread
         * of the object.  It does all the actual sending and
         * receiving of messages, using the underlying operating
         * system network handle.
         */
        void Processor();

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
         *
         * @param[in] stopProcessing
         *     This flag indicates whether or not the worker thread
         *     should be joined if it's running.
         */
        void Close(bool stopProcessing);

        /**
         * This is a helper free function which determines the IPv4
         * addresses of all active network interfaces on the local host.
         *
         * @return
         *     The IPv4 addresses of all active network interfaces on
         *     the local host are returned.
         */
        static std::vector< uint32_t > GetInterfaceAddresses();
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP */
