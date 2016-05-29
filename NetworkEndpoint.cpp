/**
 * @file NetworkEndpoint.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::NetworkEndpoint class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "NetworkEndpoint.hpp"
#include "NetworkEndpointImpl.hpp"

#include <assert.h>
#include <inttypes.h>
#include <memory>

namespace SystemAbstractions {

    NetworkEndpoint::NetworkEndpoint()
        : _impl(new NetworkEndpointImpl())
    {
    }


    NetworkEndpoint::NetworkEndpoint(NetworkEndpoint&& other)
        : _impl(std::move(other._impl))
    {
    }

    NetworkEndpoint::~NetworkEndpoint() {
    }

    NetworkEndpoint& NetworkEndpoint::operator=(NetworkEndpoint&& other) {
        assert(this != &other);
        _impl = std::move(other._impl);
        return *this;
    }

    void NetworkEndpoint::SubscribeToDiagnostics(DiagnosticsReceiver* subscriber, size_t minLevel) {
        _impl->diagnosticsSender.SubscribeToDiagnostics(subscriber, minLevel);
    }

    void NetworkEndpoint::UnsubscribeFromDiagnostics(DiagnosticsReceiver* subscriber) {
        _impl->diagnosticsSender.UnsubscribeFromDiagnostics(subscriber);
    }

    bool NetworkEndpoint::ListenForConnections(
        Owner* owner,
        uint32_t address,
        uint16_t port
    ) {
        _impl->owner = owner;
        _impl->address = address;
        _impl->port = port;
        return _impl->ListenForConnections();
    }

    uint32_t NetworkEndpoint::GetAddress() const {
        return _impl->address;
    }

    uint16_t NetworkEndpoint::GetPortNumber() const {
        return _impl->port;
    }

    void NetworkEndpoint::Close() {
        _impl->Close();
    }

}
