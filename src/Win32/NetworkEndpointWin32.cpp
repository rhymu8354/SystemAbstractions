/**
 * @file NetworkEndpointWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::NetworkEndpointPlatform class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

/**
 * WinSock2.h should always be included first because if Windows.h is
 * included before it, WinSock.h gets included which conflicts
 * with WinSock2.h.
 *
 * Windows.h should always be included next because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h beforehand.
 */
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <IPHlpApi.h>
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "IPHlpApi")
#undef ERROR
#undef SendMessage
#undef min
#undef max

#include <SystemAbstractions/NetworkConnection.hpp>
#include <SystemAbstractions/NetworkConnectionImpl.hpp>
#include <SystemAbstractions/NetworkEndpointImpl.hpp>
#include "NetworkConnectionWin32.hpp"
#include "NetworkEndpointWin32.hpp"

#include <assert.h>
#include <inttypes.h>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <thread>

namespace {

    static const size_t MAXIMUM_READ_SIZE = 65536;

}

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
        Close(true);
        if (platform->socketEvent != NULL) {
            (void)CloseHandle(platform->socketEvent);
        }
        if (platform->processorStateChangeEvent != NULL) {
            (void)CloseHandle(platform->processorStateChangeEvent);
        }
        if (platform->wsaStarted) {
            (void)WSACleanup();
        }
    }

    bool NetworkEndpointImpl::Open() {
        // Close endpoint if it was previously open.
        Close(true);

        // Obtain socket.
        platform->sock = socket(
            AF_INET,
            (mode == NetworkEndpoint::Mode::Connection) ? SOCK_STREAM : SOCK_DGRAM,
            0
        );
        if (platform->sock == INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error creating socket (%d)",
                WSAGetLastError()
            );
            return false;
        }

        // If in multicast sender mode, use local address as
        // interface socket option.  Otherwise, bind a local address
        // to the socket, and either configure group membership if in
        // multicast receive mode or obtain locally bound port otherwise.
        if (mode == NetworkEndpoint::Mode::MulticastSend) {
            struct in_addr multicastInterface;
            multicastInterface.S_un.S_addr = htonl(localAddress);
            if (setsockopt(platform->sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&multicastInterface, sizeof(multicastInterface)) == SOCKET_ERROR) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error setting socket option IP_MULTICAST_IF (%d)",
                    WSAGetLastError()
                );
                Close(false);
                return false;
            }
        } else {
            struct sockaddr_in socketAddress;
            (void)memset(&socketAddress, 0, sizeof(socketAddress));
            socketAddress.sin_family = AF_INET;
            if (mode == NetworkEndpoint::Mode::MulticastReceive) {
                BOOL option = TRUE;
                if (setsockopt(platform->sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) == SOCKET_ERROR) {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                        "error setting socket option SO_REUSEADDR (%d)",
                        WSAGetLastError()
                    );
                    Close(false);
                    return false;
                }
                socketAddress.sin_addr.S_un.S_addr = INADDR_ANY;
            } else {
                socketAddress.sin_addr.S_un.S_addr = htonl(localAddress);
            }
            socketAddress.sin_port = htons(port);
            if (bind(platform->sock, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) != 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error in bind (%d)",
                    WSAGetLastError()
                );
                Close(false);
                return false;
            }
            if (mode == NetworkEndpoint::Mode::MulticastReceive) {
                for (auto localAddress: NetworkEndpoint::GetInterfaceAddresses()) {
                    struct ip_mreq multicastGroup;
                    multicastGroup.imr_multiaddr.S_un.S_addr = htonl(groupAddress);
                    multicastGroup.imr_interface.S_un.S_addr = htonl(localAddress);
                    if (setsockopt(platform->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&multicastGroup, sizeof(multicastGroup)) == SOCKET_ERROR) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                            "error setting socket option IP_ADD_MEMBERSHIP (%d) for local interface %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
                            WSAGetLastError(),
                            (uint8_t)((localAddress >> 24) & 0xFF),
                            (uint8_t)((localAddress >> 16) & 0xFF),
                            (uint8_t)((localAddress >> 8) & 0xFF),
                            (uint8_t)(localAddress & 0xFF)
                        );
                        Close(false);
                        return false;
                    }
                }
            } else {
                int socketAddressLength = sizeof(socketAddress);
                if (getsockname(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0) {
                    port = ntohs(socketAddress.sin_port);
                } else {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                        "error in getsockname (%d)",
                        WSAGetLastError()
                    );
                    Close(false);
                    return false;
                }
            }
        }

        // Prepare events used in processing.
        if (platform->processorStateChangeEvent == NULL) {
            platform->processorStateChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->processorStateChangeEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error creating processor state change event (%d)",
                    (int)GetLastError()
                );
                Close(false);
                return false;
            }
        } else {
            (void)ResetEvent(platform->processorStateChangeEvent);
        }
        platform->processorStop = false;
        if (platform->socketEvent == NULL) {
            platform->socketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->socketEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error creating incoming client event (%d)",
                    (int)GetLastError()
                );
                Close(false);
                return false;
            }
        }
        long socketEvents = 0;
        if (mode == NetworkEndpoint::Mode::Connection) {
            socketEvents |= FD_ACCEPT;
        }
        if (
            (mode == NetworkEndpoint::Mode::Datagram)
            || (mode == NetworkEndpoint::Mode::MulticastReceive)
        ) {
            socketEvents |= FD_READ;
        }
        if (
            (mode == NetworkEndpoint::Mode::Datagram)
            || (mode == NetworkEndpoint::Mode::MulticastSend)
        ) {
            socketEvents |= FD_WRITE;
        }
        if (WSAEventSelect(platform->sock, platform->socketEvent, socketEvents) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error in WSAEventSelect (%d)",
                WSAGetLastError()
            );
            Close(false);
            return false;
        }

        // If accepting connections, tell socket to start accepting.
        if (mode == NetworkEndpoint::Mode::Connection) {
            if (listen(platform->sock, SOMAXCONN) != 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error in listen (%d)",
                    WSAGetLastError()
                );
                Close(false);
                return false;
            }
        }
        diagnosticsSender.SendDiagnosticInformationFormatted(
            0,
            "endpoint opened for port %" PRIu16,
            port
        );
        platform->processor = std::move(std::thread(&NetworkEndpointImpl::Processor, this));
        return true;
    }

    void NetworkEndpointImpl::Processor() {
        const HANDLE handles[2] = { platform->processorStateChangeEvent, platform->socketEvent };
        std::vector< uint8_t > buffer;
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        bool wait = true;
        while (!platform->processorStop) {
            if (wait) {
                processingLock.unlock();
                (void)WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                processingLock.lock();
            }
            wait = true;
            buffer.resize(MAXIMUM_READ_SIZE);
            struct sockaddr_in socketAddress;
            int socketAddressSize = sizeof(socketAddress);
            if (mode == NetworkEndpoint::Mode::Connection) {
                const SOCKET client = accept(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressSize);
                if (client == INVALID_SOCKET) {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError != WSAEWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                            "error in accept (%d)",
                            WSAGetLastError()
                        );
                    }
                } else {
                    auto connectionImpl = std::make_shared< NetworkConnectionImpl >();
                    connectionImpl->platform->sock = client;
                    connectionImpl->peerAddress = ntohl(socketAddress.sin_addr.S_un.S_addr);
                    connectionImpl->peerPort = ntohs(socketAddress.sin_port);
                    newConnectionDelegate(std::make_shared< NetworkConnection >(connectionImpl));
                }
            } else if (
                (mode == NetworkEndpoint::Mode::Datagram)
                || (mode == NetworkEndpoint::Mode::MulticastReceive)
            ) {
                const int amountReceived = recvfrom(
                    platform->sock,
                    (char*)&buffer[0],
                    (int)buffer.size(),
                    0,
                    (struct sockaddr*)&socketAddress,
                    &socketAddressSize
                );
                if (amountReceived == SOCKET_ERROR) {
                    const auto errorCode = WSAGetLastError();
                    if (errorCode != WSAEWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                            "error in recvfrom (%d)",
                            WSAGetLastError()
                        );
                        Close(false);
                        break;
                    }
                } else if (amountReceived > 0) {
                    buffer.resize(amountReceived);
                    packetReceivedDelegate(
                        ntohl(socketAddress.sin_addr.S_un.S_addr),
                        ntohs(socketAddress.sin_port),
                        buffer
                    );
                }
            }
            if (!platform->outputQueue.empty()) {
                NetworkEndpointPlatform::Packet& packet = platform->outputQueue.front();
                (void)memset(&socketAddress, 0, sizeof(socketAddress));
                socketAddress.sin_family = AF_INET;
                socketAddress.sin_addr.S_un.S_addr = htonl(packet.address);
                socketAddress.sin_port = htons(packet.port);
                const int amountSent = sendto(
                    platform->sock,
                    (const char*)&packet.body[0],
                    (int)packet.body.size(),
                    0,
                    (const sockaddr*)&socketAddress,
                    sizeof(socketAddress)
                );
                if (amountSent == SOCKET_ERROR) {
                    const auto errorCode = WSAGetLastError();
                    if (errorCode != WSAEWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                            "error in sendto (%d)",
                            WSAGetLastError()
                        );
                        Close(false);
                        break;
                    }
                } else {
                    if (amountSent != (int)packet.body.size()) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                            "send truncated (%d < %d)",
                            amountSent,
                            (int)packet.body.size()
                        );
                    }
                    platform->outputQueue.pop_front();
                    if (!platform->outputQueue.empty()) {
                        wait = false;
                    }
                }
            }
        }
    }

    void NetworkEndpointImpl::SendPacket(
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ) {
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        NetworkEndpointPlatform::Packet packet;
        packet.address = address;
        packet.port = port;
        packet.body = body;
        platform->outputQueue.push_back(std::move(packet));
        (void)SetEvent(platform->processorStateChangeEvent);
    }

    void NetworkEndpointImpl::Close(bool stopProcessing) {
        if (
            stopProcessing
            && platform->processor.joinable()
        ) {
            platform->processorStop = true;
            (void)SetEvent(platform->processorStateChangeEvent);
            platform->processor.join();
            platform->outputQueue.clear();
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

    std::vector< uint32_t > NetworkEndpointImpl::GetInterfaceAddresses() {
        // Start up WinSock library.
        bool wsaStarted = false;
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
            wsaStarted = true;
        }

        // Get addresses of all network adapters.
        //
        // Recommendation of 15KB pre-allocated buffer from:
        // https://msdn.microsoft.com/en-us/library/aa365915%28v=vs.85%29.aspx
        std::vector< uint8_t > buffer(15 * 1024);
        ULONG bufferSize = (ULONG)buffer.size();
        ULONG result = GetAdaptersAddresses(AF_INET, 0, NULL, (PIP_ADAPTER_ADDRESSES)&buffer[0], &bufferSize);
        if (result == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(bufferSize);
            result = GetAdaptersAddresses(AF_INET, 0, NULL, (PIP_ADAPTER_ADDRESSES)&buffer[0], &bufferSize);
        }
        std::vector< uint32_t > addresses;
        if (result == ERROR_SUCCESS) {
            for (
                PIP_ADAPTER_ADDRESSES adapter = (PIP_ADAPTER_ADDRESSES)&buffer[0];
                adapter != NULL;
                adapter = adapter->Next
            ) {
                if (adapter->OperStatus != IfOperStatusUp) {
                    continue;
                }
                for (
                    PIP_ADAPTER_UNICAST_ADDRESS unicastAddress = adapter->FirstUnicastAddress;
                    unicastAddress != NULL;
                    unicastAddress = unicastAddress->Next
                ) {
                    struct sockaddr_in* ipAddress = (struct sockaddr_in*)unicastAddress->Address.lpSockaddr;
                    addresses.push_back(ntohl(ipAddress->sin_addr.S_un.S_addr));
                }
            }
        }

        // Clean up WinSock library.
        if (wsaStarted) {
            (void)WSACleanup();
        }

        // Return address list.
        return addresses;
    }

}
