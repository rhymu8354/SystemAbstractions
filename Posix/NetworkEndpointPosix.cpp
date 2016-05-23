/**
 * @file NetworkEndpointPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::NetworkEndpointPlatform class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../NetworkConnection.hpp"
#include "../NetworkConnectionImpl.hpp"
#include "../NetworkEndpointImpl.hpp"
#include "NetworkConnectionPosix.hpp"
#include "NetworkEndpointPosix.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <memory>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

namespace SystemAbstractions {

    NetworkEndpointImpl::NetworkEndpointImpl()
        : platform(new NetworkEndpointPlatform())
        , diagnosticsSender("NetworkEndpoint")
    {
    }


    NetworkEndpointImpl::~NetworkEndpointImpl() {
        Close();
    }

    bool NetworkEndpointImpl::Listen() {
        Close();
        if (!NetworkConnectionPlatform::Bind(platform->sock, port, diagnosticsSender)) {
            return false;
        }
        if (platform->listener.joinable()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsReceiver::Levels::WARNING,
                "already listening"
            );
            return true;
        }
        if (!platform->listenerStopSignal.Initialize()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error creating listener stop event (%s)",
                platform->listenerStopSignal.GetLastError().c_str()
            );
            return false;
        }
        platform->listenerStopSignal.Clear();
        if (listen(platform->sock, SOMAXCONN) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error in listen: %s",
                strerror(errno)
            );
            return false;
        }
        diagnosticsSender.SendDiagnosticInformationFormatted(
            0,
            "endpoint opened for port %" PRIu16,
            port
        );
        platform->listener = std::move(std::thread(&NetworkEndpointImpl::Listener, this));
        return true;
    }

    void NetworkEndpointImpl::Listener() {
        diagnosticsSender.SendDiagnosticInformationString(
            0,
            "Listener thread started"
        );
        const int listenerStopSelectHandle = platform->listenerStopSignal.GetSelectHandle();
        const int nfds = std::max(listenerStopSelectHandle, platform->sock) + 1;
        fd_set readfds;
        for (;;) {
            FD_ZERO(&readfds);
            FD_SET(platform->sock, &readfds);
            FD_SET(listenerStopSelectHandle, &readfds);
            (void)select(nfds, &readfds, NULL, NULL, NULL);
            if (FD_ISSET(listenerStopSelectHandle, &readfds) != 0) {
                break;
            }
            struct sockaddr_in socketAddress;
            socklen_t socketAddressSize = (socklen_t)sizeof(socketAddress);
            const int client = accept(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressSize);
            if (client < 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::WARNING,
                    "error in accept: %s",
                    strerror(errno)
                );
            } else {
                std::unique_ptr< NetworkConnectionImpl > connectionImpl(new NetworkConnectionImpl());
                connectionImpl->platform->sock = client;
                connectionImpl->peerAddress = ntohl(socketAddress.sin_addr.s_addr);
                connectionImpl->peerPort = ntohs(socketAddress.sin_port);
                owner->NetworkEndpointNewConnection(
                    std::move(NetworkConnection(std::move(connectionImpl)))
                );
            }
        }
        diagnosticsSender.SendDiagnosticInformationString(
            0,
            "Listener thread stopping"
        );
    }

    void NetworkEndpointImpl::Close() {
        if (platform->listener.joinable()) {
            platform->listenerStopSignal.Set();
            platform->listener.join();
        }
        if (platform->sock >= 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0,
                "closing endpoint for port %" PRIu16,
                port
            );
            (void)shutdown(platform->sock, SHUT_RDWR);
            (void)close(platform->sock);
            platform->sock = -1;
        }
    }

}
