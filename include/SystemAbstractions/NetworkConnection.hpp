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
#include "INetworkConnection.hpp"

#include <memory>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class handles the exchanging of messages between its owner and a
     * remote object, over a network.
     */
    class NetworkConnection
        : public INetworkConnection
    {
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

        // Lifecycle Management
    public:
        ~NetworkConnection() noexcept;
        NetworkConnection(const NetworkConnection&) = delete;
        NetworkConnection(NetworkConnection&& other) noexcept = delete;
        NetworkConnection& operator=(const NetworkConnection&) = delete;
        NetworkConnection& operator=(NetworkConnection&& other) noexcept = delete;

        // Public methods
    public:
        /**
         * This is an instance constructor.
         */
        NetworkConnection();

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

        // INetworkConnection
    public:
        virtual DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        ) override;
        virtual bool Connect(uint32_t peerAddress, uint16_t peerPort) override;
        virtual bool Process(
            MessageReceivedDelegate messageReceivedDelegate,
            BrokenDelegate brokenDelegate
        ) override;
        virtual uint32_t GetPeerAddress() const override;
        virtual uint16_t GetPeerPort() const override;
        virtual bool IsConnected() const override;
        virtual uint32_t GetBoundAddress() const override;
        virtual uint16_t GetBoundPort() const override;
        virtual void SendMessage(const std::vector< uint8_t >& message) override;
        virtual void Close(bool clean = false) override;

        // Private properties
    private:
        /**
         * This contains the private properties of the instance.
         */
        std::shared_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP */
