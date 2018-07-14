/**
 * @file NetworkConnection.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::NetworkConnection class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <SystemAbstractions/NetworkConnection.hpp>
#include <SystemAbstractions/NetworkConnectionImpl.hpp>

#include <inttypes.h>

namespace SystemAbstractions {

    NetworkConnection::NetworkConnection()
        : impl_(new Impl())
    {
    }

    NetworkConnection::~NetworkConnection() {
    }

    DiagnosticsSender::SubscriptionToken NetworkConnection::SubscribeToDiagnostics(DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel) {
        return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    void NetworkConnection::UnsubscribeFromDiagnostics(DiagnosticsSender::SubscriptionToken subscriptionToken) {
        impl_->diagnosticsSender.UnsubscribeFromDiagnostics(subscriptionToken);
    }

    bool NetworkConnection::Connect(uint32_t peerAddress, uint16_t peerPort) {
        impl_->Close(true);
        impl_->peerAddress = peerAddress;
        impl_->peerPort = peerPort;
        return impl_->Connect();
    }

    bool NetworkConnection::Process(Owner* owner) {
        impl_->owner = owner;
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

    void NetworkConnection::SendMessage(const std::vector< uint8_t >& message) {
        impl_->SendMessage(message);
    }

    void NetworkConnection::Close() {
        impl_->Close(true);
    }

}
