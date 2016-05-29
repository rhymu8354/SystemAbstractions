/**
 * @file NetworkEndpointWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::NetworkEndpointPlatform class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../NetworkConnection.hpp"
#include "../NetworkConnectionImpl.hpp"
#include "../NetworkEndpointImpl.hpp"
#include "NetworkConnectionWin32.hpp"
#include "NetworkEndpointWin32.hpp"

#include <assert.h>
#include <inttypes.h>
#include <memory>
#include <string.h>
#include <thread>

#include <WinSock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32")
#undef ERROR

namespace SystemAbstractions {

    NetworkEndpointImpl::NetworkEndpointImpl()
        : platform(new NetworkEndpointPlatform())
        , diagnosticsSender("NetworkEndpoint")
    {
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
            platform->wsaStarted = true;
        }
    }


    NetworkEndpointImpl::~NetworkEndpointImpl() {
        Close();
        if (platform->wsaStarted) {
            (void)WSACleanup();
        }
        if (platform->incomingClientEvent != NULL) {
            (void)CloseHandle(platform->incomingClientEvent);
        }
        if (platform->listenerStopEvent != NULL) {
            (void)CloseHandle(platform->listenerStopEvent);
        }
    }

    bool NetworkEndpointImpl::ListenForConnections() {
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
        if (platform->listenerStopEvent == NULL) {
            platform->listenerStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (platform->listenerStopEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                    "error creating listener stop event (%d)",
                    (int)GetLastError()
                );
                return false;
            }
        } else {
            (void)ResetEvent(platform->listenerStopEvent);
        }
        if (platform->incomingClientEvent == NULL) {
            platform->incomingClientEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->incomingClientEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                    "error creating incoming client event (%d)",
                    (int)GetLastError()
                );
                return false;
            }
        }
        if (WSAEventSelect(platform->sock, platform->incomingClientEvent, FD_ACCEPT) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error in WSAEventSelect (%d)",
                WSAGetLastError()
            );
            return false;
        }
        if (listen(platform->sock, SOMAXCONN) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error in listen (%d)",
                WSAGetLastError()
            );
            return false;
        }
        diagnosticsSender.SendDiagnosticInformationFormatted(
            0,
            "endpoint opened for port %" PRIu16,
            port
        );
        platform->listener = std::move(std::thread(&NetworkEndpointImpl::ConnectionListener, this));
        return true;
    }

    void NetworkEndpointImpl::ConnectionListener() {
        diagnosticsSender.SendDiagnosticInformationString(
            0,
            "Listener thread started"
        );
        const HANDLE handles[2] = { platform->listenerStopEvent, platform->incomingClientEvent };
        while (WaitForMultipleObjects(2, handles, FALSE, INFINITE) != 0) {
            struct sockaddr_in socketAddress;
            int socketAddressSize = sizeof(socketAddress);
            const SOCKET client = accept(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressSize);
            if (client == INVALID_SOCKET) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::WARNING,
                    "error in accept (%d)",
                    WSAGetLastError()
                );
            } else {
                std::unique_ptr< NetworkConnectionImpl > connectionImpl(new NetworkConnectionImpl());
                connectionImpl->platform->sock = client;
                connectionImpl->peerAddress = ntohl(socketAddress.sin_addr.S_un.S_addr);
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
            (void)SetEvent(platform->listenerStopEvent);
            platform->listener.join();
        }
        if (platform->sock != INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0,
                "closing endpoint for port %" PRIu16,
                port
            );
            (void)closesocket(platform->sock);
            platform->sock = INVALID_SOCKET;
        }
    }

}
