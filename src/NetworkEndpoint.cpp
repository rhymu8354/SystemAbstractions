/**
 * @file NetworkEndpoint.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::NetworkEndpoint class.
 *
 * © 2016-2018 by Richard Walters
 */

#include "NetworkEndpointImpl.hpp"

#include <assert.h>
#include <inttypes.h>
#include <memory>
#include <SystemAbstractions/NetworkEndpoint.hpp>

namespace SystemAbstractions {

    NetworkEndpoint::~NetworkEndpoint() = default;
    NetworkEndpoint::NetworkEndpoint(NetworkEndpoint&& other) noexcept = default;
    NetworkEndpoint& NetworkEndpoint::operator=(NetworkEndpoint&& other) noexcept = default;

    NetworkEndpoint::NetworkEndpoint()
        : impl_(new Impl())
    {
    }

    DiagnosticsSender::SubscriptionToken NetworkEndpoint::SubscribeToDiagnostics(DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel) {
        return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    void NetworkEndpoint::UnsubscribeFromDiagnostics(DiagnosticsSender::SubscriptionToken subscriptionToken) {
        impl_->diagnosticsSender.UnsubscribeFromDiagnostics(subscriptionToken);
    }

    void NetworkEndpoint::SendPacket(
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ) {
        impl_->SendPacket(address, port, body);
    }

    bool NetworkEndpoint::Open(
        NewConnectionDelegate newConnectionDelegate,
        PacketReceivedDelegate packetReceivedDelegate,
        Mode mode,
        uint32_t localAddress,
        uint32_t groupAddress,
        uint16_t port
    ) {
        impl_->newConnectionDelegate = newConnectionDelegate;
        impl_->packetReceivedDelegate = packetReceivedDelegate;
        impl_->mode = mode;
        impl_->localAddress = localAddress;
        impl_->groupAddress = groupAddress;
        impl_->port = port;
        return impl_->Open();
    }

    uint16_t NetworkEndpoint::GetBoundPort() const {
        return impl_->port;
    }

    void NetworkEndpoint::Close() {
        impl_->Close(true);
    }

    std::vector< uint32_t > NetworkEndpoint::GetInterfaceAddresses() {
        return Impl::GetInterfaceAddresses();
    }

}
