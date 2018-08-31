#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_IMPL_HPP

/**
 * @file NetworkConnectionImpl.hpp
 *
 * This module declares the SystemAbstractions::NetworkConnectionImpl
 * structure.
 *
 * © 2016-2018 by Richard Walters
 */

#include <memory>
#include <stdint.h>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/NetworkConnection.hpp>
#include <vector>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkConnection class.
     */
    struct NetworkConnection::Impl {
        // Types

        /**
         * This is used to indicate what procedure to follow
         * in order to close the connection.
         */
        enum class CloseProcedure {
            /**
             * This indicates the connection should be terminated
             * immediately without stopping the processor thread.
             */
            ImmediateDoNotStopProcessor,

            /**
             * This indicates the connection should be terminated
             * immediately, and the processor thread should be joined.
             */
            ImmediateAndStopProcessor,

            /**
             * This indicates the connection should be gracefully
             * closed, meaning all data queued to be sent should
             * first be sent by the processor thread, and then the
             * socket should be marked as no longer sending data
             * (e.g. "shutdown" or sending the FD_CLOSE condition),
             * and then finally the socket should be closed.
             */
            Graceful,
        };

        // Properties

        /**
         * This contains any platform-specific private properties
         * of the class.
         */
        std::unique_ptr< Platform > platform;

        /**
         * This is the callback issued whenever more data
         * is received from the peer of the connection.
         */
        MessageReceivedDelegate messageReceivedDelegate;

        /**
         * This is the callback issued whenever
         * the connection is broken.
         */
        BrokenDelegate brokenDelegate;

        /**
         * This is the IPv4 address of the peer, if there is a connection
         * established.
         */
        uint32_t peerAddress = 0;

        /**
         * This is the port number of the peer, if there is a connection
         * established.
         */
        uint16_t peerPort = 0;

        /**
         * This is the IPv4 address that the connection
         * object is using, if there is a connection established.
         */
        uint32_t boundAddress = 0;

        /**
         * This is the port number that the connection
         * object is using, if there is a connection established.
         */
        uint16_t boundPort = 0;

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
         * This method attempts to establish a connection to the remote peer.
         *
         * @return
         *     An indication of whether or not the connection was successfully
         *     established is returned.
         */
        bool Connect();

        /**
         * This method starts message processing on the connection,
         * listening for incoming messages and sending outgoing messages.
         *
         * @return
         *     An indication of whether or not the method was
         *     successful is returned.
         */
        bool Process();

        /**
         * This is the main function called for the worker thread
         * of the object.  It does all the actual sending and
         * receiving of messages, using the underlying operating
         * system network handle.
         */
        void Processor();

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
         * This method breaks the connection to the peer.
         *
         * @param[in] procedure
         *     This indicates the procedure to follow in order
         *     to close the connection.
         */
        void Close(CloseProcedure procedure);

        /**
         * This helper method is called from various places to standardize
         * what the class does when it wants to immediately close
         * the connection.
         */
        void CloseImmediately();

        /**
         * This method returns a string representation of the address
         * of the peer of the connection.
         *
         * @return
         *     A string representation of the address
         *     of the peer of the connection is returned.
         */
        std::string GetPeerName() const;

        /**
         * This is a helper free function which determines the IPv4
         * address of a host having the given name (which could just
         * be an IPv4 address formatted as a string).
         *
         * @return
         *     The IPv4 address of the host having the given name is returned.
         *
         * @retval 0
         *     This is returned if the IPv4 address of the host having
         *     the given name could not be determined.
         */
        static uint32_t GetAddressOfHost(const std::string& host);
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_IMPL_HPP */
