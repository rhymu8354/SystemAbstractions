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
        uint16_t port
    ) {
        _impl->owner = owner;
        _impl->port = port;
        return _impl->ListenForConnections();
    }

    uint16_t NetworkEndpoint::GetPortNumber() const {
        return _impl->port;
    }

    void NetworkEndpoint::Close() {
        _impl->Close();
    }

}
