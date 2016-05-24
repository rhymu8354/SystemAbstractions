/**
 * @file NetworkConnection.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::NetworkConnection class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "NetworkConnection.hpp"
#include "NetworkConnectionImpl.hpp"

#include <assert.h>
#include <inttypes.h>

namespace SystemAbstractions {

    NetworkConnection::NetworkConnection()
        : _impl(new NetworkConnectionImpl())
    {
    }


    NetworkConnection::NetworkConnection(NetworkConnection&& other)
        : _impl(std::move(other._impl))
    {
    }

    NetworkConnection::NetworkConnection(std::unique_ptr< NetworkConnectionImpl >&& impl)
        : _impl(std::move(impl))
    {
    }

    NetworkConnection::~NetworkConnection() {
    }

    NetworkConnection& NetworkConnection::operator=(NetworkConnection&& other) {
        assert(this != &other);
        _impl = std::move(other._impl);
        return *this;
    }

    void NetworkConnection::SubscribeToDiagnostics(DiagnosticsReceiver* subscriber, size_t minLevel) {
        _impl->diagnosticsSender.SubscribeToDiagnostics(subscriber, minLevel);
    }

    void NetworkConnection::UnsubscribeFromDiagnostics(DiagnosticsReceiver* subscriber) {
        _impl->diagnosticsSender.UnsubscribeFromDiagnostics(subscriber);
    }

    bool NetworkConnection::Connect(uint32_t peerAddress, uint16_t peerPort) {
        _impl->Close(true);
        _impl->peerAddress = peerAddress;
        _impl->peerPort = peerPort;
        return _impl->Connect();
    }

    bool NetworkConnection::Process(Owner* owner) {
        _impl->owner = owner;
        return _impl->Process();
    }

    uint32_t NetworkConnection::GetPeerAddress() const {
        return _impl->peerAddress;
    }

    uint16_t NetworkConnection::GetPeerPort() const {
        return _impl->peerPort;
    }

    void NetworkConnection::SendMessage(const std::vector< uint8_t >& message) {
        _impl->SendMessage(message);
    }

    void NetworkConnection::Close() {
        _impl->Close(true);
    }

}