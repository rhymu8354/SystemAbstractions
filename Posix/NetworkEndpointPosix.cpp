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
#include <ifaddrs.h>
#include <inttypes.h>
#include <memory>
#include <net/if.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif /* MSG_NOSIGNAL */

namespace {

    static const size_t MAXIMUM_READ_SIZE = 65536;

}

namespace SystemAbstractions {

    NetworkEndpointImpl::NetworkEndpointImpl()
        : platform(new NetworkEndpointPlatform())
        , diagnosticsSender("NetworkEndpoint")
    {
    }


    NetworkEndpointImpl::~NetworkEndpointImpl() {
        Close(true);
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
        if (platform->sock < 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error creating socket: %s",
                strerror(errno)
            );
            return false;
        }

        // If in multicast sender mode, use local address as
        // interface socket option.  Otherwise, bind a local address
        // to the socket, and either configure group membership if in
        // multicast receive mode or obtain locally bound port otherwise.
        if (mode == NetworkEndpoint::Mode::MulticastSend) {
            struct in_addr multicastInterface;
            multicastInterface.s_addr = htonl(localAddress);
            if (setsockopt(platform->sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&multicastInterface, sizeof(multicastInterface)) < 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                    "error setting socket option IP_MULTICAST_IF: %s",
                    strerror(errno)
                );
                Close(false);
                return false;
            }
        } else {
            struct sockaddr_in socketAddress;
            (void)memset(&socketAddress, 0, sizeof(socketAddress));
            socketAddress.sin_family = AF_INET;
            if (mode == NetworkEndpoint::Mode::MulticastReceive) {
                int option = 1;
                if (setsockopt(platform->sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                        "error setting socket option SO_REUSEADDR: %s",
                        strerror(errno)
                    );
                    Close(false);
                    return false;
                }
                socketAddress.sin_addr.s_addr = INADDR_ANY;
            } else {
                socketAddress.sin_addr.s_addr = htonl(localAddress);
            }
            socketAddress.sin_port = htons(port);
            if (bind(platform->sock, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) != 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                    "error in bind: %s",
                    strerror(errno)
                );
                Close(false);
                return false;
            }
            if (mode == NetworkEndpoint::Mode::MulticastReceive) {
                for (auto localAddress: NetworkEndpoint::GetInterfaceAddresses()) {
                    struct ip_mreq multicastGroup;
                    multicastGroup.imr_multiaddr.s_addr = htonl(groupAddress);
                    multicastGroup.imr_interface.s_addr = htonl(localAddress);
                    if (setsockopt(platform->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&multicastGroup, sizeof(multicastGroup)) < 0) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                            "error setting socket option IP_ADD_MEMBERSHIP: %s",
                            strerror(errno)
                        );
                        Close(false);
                        return false;
                    }
                }
            } else {
                socklen_t socketAddressLength = sizeof(socketAddress);
                if (getsockname(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0) {
                    port = ntohs(socketAddress.sin_port);
                } else {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                        "error in getsockname: %s",
                        strerror(errno)
                    );
                    Close(false);
                    return false;
                }
            }
        }

        // Prepare events used in processing.
        if (!platform->processorStateChangeSignal.Initialize()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error creating processor state change event (%s)",
                platform->processorStateChangeSignal.GetLastError().c_str()
            );
            return false;
        }
        platform->processorStateChangeSignal.Clear();

        // If accepting connections, tell socket to start accepting.
        if (mode == NetworkEndpoint::Mode::Connection) {
            if (listen(platform->sock, SOMAXCONN) != 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                    "error in listen: %s",
                    strerror(errno)
                );
                return false;
            }
        }
        diagnosticsSender.SendDiagnosticInformationFormatted(
            0,
            "endpoint opened for port %" PRIu16,
            port
        );
        platform->processorStop = false;
        platform->processor = std::thread(&NetworkEndpointImpl::Processor, this);
        return true;
    }

    void NetworkEndpointImpl::Processor() {
        const int processorStateChangeSelectHandle = platform->processorStateChangeSignal.GetSelectHandle();
        const int nfds = std::max(processorStateChangeSelectHandle, platform->sock) + 1;
        fd_set readfds, writefds;
        std::vector< uint8_t > buffer;
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        bool wait = true;
        while (!platform->processorStop) {
            if (wait) {
                FD_ZERO(&readfds);
                FD_ZERO(&writefds);
                FD_SET(platform->sock, &readfds);
                if (platform->outputQueue.size() > 0) {
                    FD_SET(platform->sock, &writefds);
                }
                FD_SET(processorStateChangeSelectHandle, &readfds);
                processingLock.unlock();
                (void)select(nfds, &readfds, &writefds, NULL, NULL);
                processingLock.lock();
                if (FD_ISSET(processorStateChangeSelectHandle, &readfds) != 0) {
                    platform->processorStateChangeSignal.Clear();
                }
            }
            wait = true;
            buffer.resize(MAXIMUM_READ_SIZE);
            struct sockaddr_in socketAddress;
            socklen_t socketAddressSize = (socklen_t)sizeof(socketAddress);
            if (FD_ISSET(platform->sock, &readfds)) {
                if (mode == NetworkEndpoint::Mode::Connection) {
                    const int client = accept(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressSize);
                    if (client < 0) {
                        if (errno != EWOULDBLOCK) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                SystemAbstractions::DiagnosticsReceiver::Levels::WARNING,
                                "error in accept: %s",
                                strerror(errno)
                            );
                        }
                    } else {
                        auto connectionImpl = std::make_shared< NetworkConnectionImpl >();
                        connectionImpl->platform->sock = client;
                        connectionImpl->peerAddress = ntohl(socketAddress.sin_addr.s_addr);
                        connectionImpl->peerPort = ntohs(socketAddress.sin_port);
                        owner->NetworkEndpointNewConnection(std::make_shared< NetworkConnection >(connectionImpl));
                    }
                } else if (
                    (mode == NetworkEndpoint::Mode::Datagram)
                    || (mode == NetworkEndpoint::Mode::MulticastReceive)
                ) {
                    const ssize_t amountReceived = recvfrom(
                        platform->sock,
                        &buffer[0],
                        buffer.size(),
                        MSG_NOSIGNAL | MSG_DONTWAIT,
                        (struct sockaddr*)&socketAddress,
                        &socketAddressSize
                    );
                    if (amountReceived < 0) {
                        if (errno != EWOULDBLOCK) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                                "error in recvfrom: %s",
                                strerror(errno)
                            );
                            Close(false);
                            break;
                        }
                    } else if (amountReceived > 0) {
                        buffer.resize((size_t)amountReceived);
                        owner->NetworkEndpointPacketReceived(
                            ntohl(socketAddress.sin_addr.s_addr),
                            ntohs(socketAddress.sin_port),
                            buffer
                        );
                    }
                }
            }
            if (!platform->outputQueue.empty()) {
                NetworkEndpointPlatform::Packet& packet = platform->outputQueue.front();
                (void)memset(&socketAddress, 0, sizeof(socketAddress));
                socketAddress.sin_family = AF_INET;
                socketAddress.sin_addr.s_addr = htonl(packet.address);
                socketAddress.sin_port = htons(packet.port);
                const ssize_t amountSent = sendto(
                    platform->sock,
                    &packet.body[0],
                    packet.body.size(),
                    MSG_NOSIGNAL | MSG_DONTWAIT,
                    (const sockaddr*)&socketAddress,
                    sizeof(socketAddress)
                );
                if (amountSent < 0) {
                    if (errno != EWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                            "error in sendto: %s",
                            strerror(errno)
                        );
                        Close(false);
                        break;
                    }
                } else {
                    if ((size_t)amountSent != packet.body.size()) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                            "send truncated (%d < %d)",
                            (int)amountSent,
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
        platform->outputQueue.emplace_back(std::move(packet));
        platform->processorStateChangeSignal.Set();
    }

    void NetworkEndpointImpl::Close(bool stopProcessing) {
        if (
            stopProcessing
            && platform->processor.joinable()
        ) {
            platform->processorStop = true;
            platform->processorStateChangeSignal.Set();
            platform->processor.join();
        }
        if (platform->sock >= 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0,
                "closing endpoint for port %" PRIu16,
                port
            );
            (void)close(platform->sock);
            platform->sock = -1;
        }
    }

    std::vector< uint32_t > NetworkEndpointImpl::GetInterfaceAddresses() {
        std::vector< uint32_t > addresses;
        struct ifaddrs* ifaddrHead;
        if (getifaddrs(&ifaddrHead) < 0) {
            return addresses;
        }
        for (
            struct ifaddrs* ifaddr = ifaddrHead;
            ifaddr != NULL;
            ifaddr = ifaddr->ifa_next
        ) {
            if ((ifaddr->ifa_flags & IFF_UP) == 0) {
                continue;
            }
            if (
                (ifaddr->ifa_addr != NULL)
                && (ifaddr->ifa_addr->sa_family == AF_INET)
            ) {
                struct sockaddr_in* ipAddress = (struct sockaddr_in*)ifaddr->ifa_addr;
                addresses.push_back(ntohl(ipAddress->sin_addr.s_addr));
            }
        }
        freeifaddrs(ifaddrHead);
        return addresses;
    }

}
