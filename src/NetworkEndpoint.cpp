/**
 * @file NetworkEndpoint.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::NetworkEndpoint class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <SystemAbstractions/NetworkEndpoint.hpp>
#include <SystemAbstractions/NetworkEndpointImpl.hpp>

#include <assert.h>
#include <inttypes.h>
#include <memory>

namespace SystemAbstractions {

    NetworkEndpoint::NetworkEndpoint()
        : _impl(new NetworkEndpointImpl())
    {
    }


    NetworkEndpoint::NetworkEndpoint(NetworkEndpoint&& other) noexcept
        : _impl(std::move(other._impl))
    {
    }

    NetworkEndpoint::~NetworkEndpoint() {
    }

    NetworkEndpoint& NetworkEndpoint::operator=(NetworkEndpoint&& other) noexcept {
        _impl = std::move(other._impl);
        return *this;
    }

    DiagnosticsSender::SubscriptionToken NetworkEndpoint::SubscribeToDiagnostics(DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel) {
        return _impl->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    void NetworkEndpoint::UnsubscribeFromDiagnostics(DiagnosticsSender::SubscriptionToken subscriptionToken) {
        _impl->diagnosticsSender.UnsubscribeFromDiagnostics(subscriptionToken);
    }

    void NetworkEndpoint::SendPacket(
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ) {
        _impl->SendPacket(address, port, body);
    }

    bool NetworkEndpoint::Open(
        Owner* owner,
        Mode mode,
        uint32_t localAddress,
        uint32_t groupAddress,
        uint16_t port
    ) {
        _impl->owner = owner;
        _impl->mode = mode;
        _impl->localAddress = localAddress;
        _impl->groupAddress = groupAddress;
        _impl->port = port;
        return _impl->Open();
    }

    uint16_t NetworkEndpoint::GetBoundPort() const {
        return _impl->port;
    }

    void NetworkEndpoint::Close() {
        _impl->Close(true);
    }

    std::vector< uint32_t > NetworkEndpoint::GetInterfaceAddresses() {
        return NetworkEndpointImpl::GetInterfaceAddresses();
    }

}
