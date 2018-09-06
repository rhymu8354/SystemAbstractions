/**
 * @file NetworkConnectionPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::NetworkConnection::Impl class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../NetworkConnectionImpl.hpp"
#include "NetworkConnectionPosix.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
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
    static const size_t MAXIMUM_WRITE_SIZE = 65536;

}

namespace SystemAbstractions {

    NetworkConnection::Impl::Impl()
        : platform(new Platform())
        , diagnosticsSender("NetworkConnection")
    {
    }

    NetworkConnection::Impl::~Impl() {
        Close(CloseProcedure::ImmediateAndStopProcessor);
    }

    bool NetworkConnection::Impl::Connect() {
        Close(CloseProcedure::ImmediateAndStopProcessor);
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        platform->sock = socket(socketAddress.sin_family, SOCK_STREAM, 0);
        if (platform->sock < 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error creating socket: %s",
                strerror(errno)
            );
            return false;
        }
        struct linger linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;
        (void)setsockopt(platform->sock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
        if (bind(platform->sock, (struct sockaddr*)&socketAddress, (socklen_t)sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error in bind: %s",
                strerror(errno)
            );
            Close(CloseProcedure::ImmediateDoNotStopProcessor);
            return false;
        }
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.s_addr = htonl(peerAddress);
        socketAddress.sin_port = htons(peerPort);
        if (connect(platform->sock, (const sockaddr*)&socketAddress, (socklen_t)sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error in connect: %s",
                strerror(errno)
            );
            Close(CloseProcedure::ImmediateDoNotStopProcessor);
            return false;
        }
        socklen_t socketAddressLength = sizeof(socketAddress);
        if (getsockname(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0) {
            boundAddress = ntohl(socketAddress.sin_addr.s_addr);
            boundPort = ntohs(socketAddress.sin_port);
        }
        return true;
    }

    bool NetworkConnection::Impl::Process() {
        if (platform->sock < 0) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "not connected"
            );
            return false;
        }
#ifdef SO_NOSIGPIPE
        int opt = 1;
        if (setsockopt(platform->sock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt)) < 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                "error in setsockopt(SO_NOSIGPIPE): %s",
                strerror(errno)
            );
        }
#endif /* SO_NOSIGPIPE */
        if (platform->processor.joinable()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                "already processing"
            );
            return true;
        }
        platform->processorStop = false;
        if (!platform->processorStateChangeSignal.Initialize()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error creating processor state change event: %s",
                platform->processorStateChangeSignal.GetLastError().c_str()
            );
            return false;
        }
        platform->processorStateChangeSignal.Clear();
        platform->processor = std::thread(&NetworkConnection::Impl::Processor, this);
        return true;
    }

    void NetworkConnection::Impl::Processor() {
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
                if (platform->outputQueue.GetBytesQueued() > 0) {
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
            if (platform->peerClosed) {
                wait = true;
            } else {
                buffer.resize(MAXIMUM_READ_SIZE);
                const auto amountReceived = recv(platform->sock, (char*)&buffer[0], (int)buffer.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
                if (amountReceived < 0) {
                    if (errno == EWOULDBLOCK) {
                        wait = true;
                    } else {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1,
                            "connection with " + GetPeerName() + " closed abruptly by peer"
                        );
                        Close(CloseProcedure::ImmediateDoNotStopProcessor);
                        break;
                    }
                } else if (amountReceived > 0) {
                    buffer.resize((size_t)amountReceived);
                    wait = false;
                    messageReceivedDelegate(buffer);
                } else {
                    diagnosticsSender.SendDiagnosticInformationString(
                        1,
                        "connection with " + GetPeerName() + " closed gracefully by peer"
                    );
                    platform->peerClosed = true;
                    brokenDelegate(true);
                }
            }
            const auto outputQueueLength = platform->outputQueue.GetBytesQueued();
            if (outputQueueLength > 0) {
                const auto writeSize = (int)std::min(outputQueueLength, MAXIMUM_WRITE_SIZE);
                buffer = platform->outputQueue.Peek(writeSize);
                const auto amountSent = send(platform->sock, (const char*)&buffer[0], writeSize, MSG_NOSIGNAL | MSG_DONTWAIT);
                if (amountSent < 0) {
                    if (errno != EWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1,
                            "connection with " + GetPeerName() + " closed abruptly by peer"
                        );
                        Close(CloseProcedure::ImmediateDoNotStopProcessor);
                        break;
                    }
                } else if (amountSent > 0) {
                    (void)platform->outputQueue.Drop(amountSent);
                    if (
                        (amountSent == writeSize)
                        && (platform->outputQueue.GetBytesQueued() > 0)
                    ) {
                        wait = false;
                    }
                } else {
                    Close(CloseProcedure::ImmediateDoNotStopProcessor);
                    break;
                }
            }
            if (
                (platform->outputQueue.GetBytesQueued() == 0)
                && platform->closing
            ) {
                if (!platform->shutdownSent) {
                    shutdown(platform->sock, SHUT_WR);
                    platform->shutdownSent = true;
                }
                if (platform->peerClosed) {
                    CloseImmediately();
                }
            }
        }
    }

    bool NetworkConnection::Impl::IsConnected() const {
        return (platform->sock >= 0);
    }

    void NetworkConnection::Impl::SendMessage(const std::vector< uint8_t >& message) {
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        platform->outputQueue.Enqueue(message);
        platform->processorStateChangeSignal.Set();
    }

    void NetworkConnection::Impl::Close(CloseProcedure procedure) {
        if (
            (procedure == CloseProcedure::ImmediateAndStopProcessor)
            && platform->processor.joinable()
            && (std::this_thread::get_id() != platform->processor.get_id())
        ) {
            platform->processorStop = true;
            platform->processorStateChangeSignal.Set();
            platform->processor.join();
        }
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        if (platform->sock >= 0) {
            if (procedure == CloseProcedure::Graceful) {
                platform->closing = true;
                diagnosticsSender.SendDiagnosticInformationString(
                    1,
                    "closing connection with " + GetPeerName()
                );
                platform->processorStateChangeSignal.Set();
            } else {
                CloseImmediately();
            }
        }
    }

    void NetworkConnection::Impl::CloseImmediately() {
        platform->CloseImmediately();
        diagnosticsSender.SendDiagnosticInformationString(
            1,
            "closed connection with " + GetPeerName()
        );
        if (brokenDelegate != nullptr) {
            brokenDelegate(false);
        }
    }

    uint32_t NetworkConnection::Impl::GetAddressOfHost(const std::string& host) {
        struct addrinfo hints;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        struct addrinfo* rawResults;
        if (getaddrinfo(host.c_str(), NULL, &hints, &rawResults) != 0) {
            return 0;
        }
        std::unique_ptr< struct addrinfo, std::function< void(struct addrinfo*) > > results(
            rawResults,
            [](struct addrinfo* p){
                freeaddrinfo(p);
            }
        );
        if (results == NULL) {
            return 0;
        } else {
            struct sockaddr_in* ipAddress = (struct sockaddr_in*)results->ai_addr;
            return ntohl(ipAddress->sin_addr.s_addr);
        }
    }

    std::shared_ptr< NetworkConnection > NetworkConnection::Platform::MakeConnectionFromExistingSocket(
        int sock,
        uint32_t boundAddress,
        uint16_t boundPort,
        uint32_t peerAddress,
        uint16_t peerPort
    ) {
        const auto connection = std::make_shared< NetworkConnection >();
        connection->impl_->platform->sock = sock;
        connection->impl_->boundAddress = boundAddress;
        connection->impl_->boundPort = boundPort;
        connection->impl_->peerAddress = peerAddress;
        connection->impl_->peerPort = peerPort;
        return connection;
    }

    void NetworkConnection::Platform::CloseImmediately() {
        (void)close(sock);
        sock = -1;
    }

}
