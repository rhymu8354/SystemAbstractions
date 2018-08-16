/**
 * @file NetworkEndpointPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::NetworkEndpoint::Platform class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

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
#include <SystemAbstractions/NetworkConnection.hpp>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif /* MSG_NOSIGNAL */

namespace {

    static const size_t MAXIMUM_READ_SIZE = 65536;

}

namespace SystemAbstractions {

    NetworkEndpoint::Impl::Impl()
        : platform(new Platform())
        , diagnosticsSender("NetworkEndpoint")
    {
    }


    NetworkEndpoint::Impl::~Impl() {
        Close(true);
    }

    bool NetworkEndpoint::Impl::Open() {
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
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
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
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error setting socket option IP_MULTICAST_IF: %s",
                    strerror(errno)
                );
                Close(false);
                return false;
            }
        } else {
            struct sockaddr_in peerAddress;
            (void)memset(&peerAddress, 0, sizeof(peerAddress));
            peerAddress.sin_family = AF_INET;
            if (mode == NetworkEndpoint::Mode::MulticastReceive) {
                int option = 1;
                if (setsockopt(platform->sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                        "error setting socket option SO_REUSEADDR: %s",
                        strerror(errno)
                    );
                    Close(false);
                    return false;
                }
                peerAddress.sin_addr.s_addr = INADDR_ANY;
            } else {
                peerAddress.sin_addr.s_addr = htonl(localAddress);
            }
            peerAddress.sin_port = htons(port);
            if (bind(platform->sock, (struct sockaddr*)&peerAddress, sizeof(peerAddress)) != 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
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
                            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                            "error setting socket option IP_ADD_MEMBERSHIP: %s",
                            strerror(errno)
                        );
                        Close(false);
                        return false;
                    }
                }
            } else {
                socklen_t peerAddressLength = sizeof(peerAddress);
                if (getsockname(platform->sock, (struct sockaddr*)&peerAddress, &peerAddressLength) == 0) {
                    port = ntohs(peerAddress.sin_port);
                } else {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::ERROR,
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
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
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
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
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
        platform->processor = std::thread(&NetworkEndpoint::Impl::Processor, this);
        return true;
    }

    void NetworkEndpoint::Impl::Processor() {
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
            struct sockaddr_in peerAddress;
            socklen_t peerAddressSize = (socklen_t)sizeof(peerAddress);
            if (FD_ISSET(platform->sock, &readfds)) {
                if (mode == NetworkEndpoint::Mode::Connection) {
                    const int client = accept(platform->sock, (struct sockaddr*)&peerAddress, &peerAddressSize);
                    if (client < 0) {
                        if (errno != EWOULDBLOCK) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                                "error in accept: %s",
                                strerror(errno)
                            );
                        }
                    } else {
                        struct linger linger;
                        linger.l_onoff = 1;
                        linger.l_linger = 0;
                        (void)setsockopt(client, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
                        uint32_t boundIpv4Address = 0;
                        uint16_t boundPort = 0;
                        struct sockaddr_in boundAddress;
                        socklen_t boundAddressSize = sizeof(boundAddress);
                        if (getsockname(client, (struct sockaddr*)&boundAddress, &boundAddressSize) == 0) {
                            boundIpv4Address = ntohl(boundAddress.sin_addr.s_addr);
                            boundPort = ntohs(boundAddress.sin_port);
                        }
                        auto connection = NetworkConnection::Platform::MakeConnectionFromExistingSocket(
                            client,
                            boundIpv4Address,
                            boundPort,
                            ntohl(peerAddress.sin_addr.s_addr),
                            ntohs(peerAddress.sin_port)
                        );
                        newConnectionDelegate(connection);
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
                        (struct sockaddr*)&peerAddress,
                        &peerAddressSize
                    );
                    if (amountReceived < 0) {
                        if (errno != EWOULDBLOCK) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                                "error in recvfrom: %s",
                                strerror(errno)
                            );
                            Close(false);
                            break;
                        }
                    } else if (amountReceived > 0) {
                        buffer.resize((size_t)amountReceived);
                        packetReceivedDelegate(
                            ntohl(peerAddress.sin_addr.s_addr),
                            ntohs(peerAddress.sin_port),
                            buffer
                        );
                    }
                }
            }
            if (!platform->outputQueue.empty()) {
                NetworkEndpoint::Platform::Packet& packet = platform->outputQueue.front();
                (void)memset(&peerAddress, 0, sizeof(peerAddress));
                peerAddress.sin_family = AF_INET;
                peerAddress.sin_addr.s_addr = htonl(packet.address);
                peerAddress.sin_port = htons(packet.port);
                const ssize_t amountSent = sendto(
                    platform->sock,
                    &packet.body[0],
                    packet.body.size(),
                    MSG_NOSIGNAL | MSG_DONTWAIT,
                    (const sockaddr*)&peerAddress,
                    sizeof(peerAddress)
                );
                if (amountSent < 0) {
                    if (errno != EWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                            "error in sendto: %s",
                            strerror(errno)
                        );
                        Close(false);
                        break;
                    }
                } else {
                    if ((size_t)amountSent != packet.body.size()) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
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

    void NetworkEndpoint::Impl::SendPacket(
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ) {
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        NetworkEndpoint::Platform::Packet packet;
        packet.address = address;
        packet.port = port;
        packet.body = body;
        platform->outputQueue.emplace_back(std::move(packet));
        platform->processorStateChangeSignal.Set();
    }

    void NetworkEndpoint::Impl::Close(bool stopProcessing) {
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

    std::vector< uint32_t > NetworkEndpoint::Impl::GetInterfaceAddresses() {
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
