/**
 * @file NetworkConnection.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::NetworkConnection class.
 *
 * © 2016-2018 by Richard Walters
 */

#include "NetworkConnectionImpl.hpp"

#include <inttypes.h>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/NetworkConnection.hpp>

namespace SystemAbstractions {

    NetworkConnection::~NetworkConnection() noexcept {
        Close(false);
    }

    NetworkConnection::NetworkConnection()
        : impl_(new Impl())
    {
    }

    DiagnosticsSender::UnsubscribeDelegate NetworkConnection::SubscribeToDiagnostics(DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel) {
        return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    bool NetworkConnection::Connect(uint32_t peerAddress, uint16_t peerPort) {
        impl_->peerAddress = peerAddress;
        impl_->peerPort = peerPort;
        return impl_->Connect();
    }

    bool NetworkConnection::Process(
        MessageReceivedDelegate messageReceivedDelegate,
        BrokenDelegate brokenDelegate
    ) {
        impl_->messageReceivedDelegate = messageReceivedDelegate;
        impl_->brokenDelegate = brokenDelegate;
        return impl_->Process();
    }

    uint32_t NetworkConnection::GetPeerAddress() const {
        return impl_->peerAddress;
    }

    uint16_t NetworkConnection::GetPeerPort() const {
        return impl_->peerPort;
    }

    bool NetworkConnection::IsConnected() const {
        return impl_->IsConnected();
    }

    uint32_t NetworkConnection::GetBoundAddress() const {
        return impl_->boundAddress;
    }

    uint16_t NetworkConnection::GetBoundPort() const {
        return impl_->boundPort;
    }

    void NetworkConnection::SendMessage(const std::vector< uint8_t >& message) {
        impl_->SendMessage(message);
    }

    void NetworkConnection::Close(bool clean) {
        if (
            impl_->Close(
                clean
                ? Impl::CloseProcedure::Graceful
                : Impl::CloseProcedure::ImmediateAndStopProcessor
            )
        ) {
            impl_->brokenDelegate(false);
        }
    }

    uint32_t NetworkConnection::GetAddressOfHost(const std::string& host) {
        return Impl::GetAddressOfHost(host);
    }

}
