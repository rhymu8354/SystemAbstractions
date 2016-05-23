/**
 * @file NetworkConnectionPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::NetworkConnectionImpl class.
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
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

namespace {

    static const size_t MAXIMUM_READ_SIZE = 65536;
    static const size_t MAXIMUM_WRITE_SIZE = 65536;

}

namespace SystemAbstractions {

    bool NetworkConnectionPlatform::Bind(
        int& sock,
        uint16_t port,
        DigitalStirling::DiagnosticsSender& diagnosticsSender
    ) {
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_port = htons(port);
        sock = socket(socketAddress.sin_family, SOCK_STREAM, 0);
        if (sock < 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                DigitalStirling::DiagnosticsReceiver::Levels::ERROR,
                "error creating socket: %s",
                strerror(errno)
            );
            return false;
        }
        if (bind(sock, (struct sockaddr*)&socketAddress, (socklen_t)sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                DigitalStirling::DiagnosticsReceiver::Levels::ERROR,
                "error in bind: %s",
                strerror(errno)
            );
            (void)close(sock);
            sock = -1;
            return false;
        }
        return true;
    }

    NetworkConnectionImpl::NetworkConnectionImpl()
        : platform(new NetworkConnectionPlatform())
        , diagnosticsSender("NetworkConnection")
    {
    }

    NetworkConnectionImpl::~NetworkConnectionImpl() {
        Close(true);
    }

    bool NetworkConnectionImpl::Connect() {
        Close(true);
        if (!NetworkConnectionPlatform::Bind(platform->sock, 0, diagnosticsSender)) {
            return false;
        }
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.s_addr = htonl(peerAddress);
        socketAddress.sin_port = htons(peerPort);
        if (connect(platform->sock, (const sockaddr*)&socketAddress, (socklen_t)sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                DigitalStirling::DiagnosticsReceiver::Levels::ERROR,
                "error in connect: %s",
                strerror(errno)
            );
            Close(false);
            return false;
        }
        return true;
    }

    bool NetworkConnectionImpl::Process() {
        if (platform->sock < 0) {
            diagnosticsSender.SendDiagnosticInformationString(
                DigitalStirling::DiagnosticsReceiver::Levels::ERROR,
                "not connected"
            );
            return false;
        }
        if (platform->processor.joinable()) {
            diagnosticsSender.SendDiagnosticInformationString(
                DigitalStirling::DiagnosticsReceiver::Levels::WARNING,
                "already processing"
            );
            return true;
        }
        platform->processorStop = false;
        if (!platform->processorStateChangeSignal.Initialize()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                DigitalStirling::DiagnosticsReceiver::Levels::ERROR,
                "error creating processor state change event: %s",
                platform->processorStateChangeSignal.GetLastError().c_str()
            );
            return false;
        }
        platform->processorStateChangeSignal.Clear();
        platform->processor = std::move(std::thread(&NetworkConnectionImpl::Processor, this));
        return true;
    }

    void NetworkConnectionImpl::Processor() {
        diagnosticsSender.SendDiagnosticInformationFormatted(
            0,
            "Processing started for connection with %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16,
            (uint8_t)((peerAddress >> 24) & 0xFF),
            (uint8_t)((peerAddress >> 16) & 0xFF),
            (uint8_t)((peerAddress >> 8) & 0xFF),
            (uint8_t)(peerAddress & 0xFF),
            peerPort
        );
        const int processorStateChangeSelectHandle = platform->processorStateChangeSignal.GetSelectHandle();
        const int nfds = std::max(processorStateChangeSelectHandle, platform->sock) + 1;
        fd_set readfds, writefds;
        std::vector< uint8_t > buffer;
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        bool wait = true;
        while (!platform->processorStop) {
            if (wait) {
                FD_ZERO(&readfds);
                FD_SET(platform->sock, &readfds);
                if (platform->outputQueue.size() > 0) {
                    FD_SET(platform->sock, &writefds);
                }
                FD_SET(processorStateChangeSelectHandle, &readfds);
                processingLock.unlock();
                (void)select(nfds, &readfds, NULL, NULL, NULL);
                processingLock.lock();
                std::string wakeReasons;
                if (FD_ISSET(platform->sock, &readfds)) {
                    wakeReasons += 'R';
                }
                if (FD_ISSET(platform->sock, &writefds)) {
                    wakeReasons += 'W';
                }
                if (FD_ISSET(processorStateChangeSelectHandle, &readfds)) {
                    wakeReasons += 'S';
                }
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    0,
                    "processor awakened: %s",
                    wakeReasons.c_str()
                );
                if (FD_ISSET(processorStateChangeSelectHandle, &readfds) != 0) {
                    platform->processorStateChangeSignal.Clear();
                }
            }
            wait = true;
            buffer.resize(MAXIMUM_READ_SIZE);
            const auto amountReceived = recv(platform->sock, (char*)&buffer[0], (int)buffer.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
            if (amountReceived < 0) {
                if (errno == EWOULDBLOCK) {
                    diagnosticsSender.SendDiagnosticInformationString(
                        0,
                        "read not ready"
                    );
                } else {
                    if (errno != ECONNRESET) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            0,
                            "read error: %s",
                            strerror(errno)
                        );
                    }
                    Close(false);
                    owner->NetworkConnectionBroken();
                    break;
                }
            } else if (amountReceived > 0) {
                buffer.resize((size_t)amountReceived);
                owner->NetworkConnectionMessageReceived(buffer);
            } else {
                diagnosticsSender.SendDiagnosticInformationString(
                    0,
                    "zero bytes read"
                );
                Close(false);
                owner->NetworkConnectionBroken();
                break;
            }
            const auto outputQueueLength = platform->outputQueue.size();
            if (outputQueueLength > 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    0,
                    "output queue length: %u",
                    (unsigned int)outputQueueLength
                );
                const auto writeSize = (int)std::min(outputQueueLength, MAXIMUM_WRITE_SIZE);
                buffer.assign(
                    platform->outputQueue.begin(),
                    platform->outputQueue.begin() + writeSize
                );
                const auto amountSent = send(platform->sock, (const char*)&buffer[0], writeSize, MSG_NOSIGNAL | MSG_DONTWAIT);
                if (amountSent < 0) {
                    if (errno == EWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationString(
                            0,
                            "write not ready"
                        );
                    } else {
                        if (errno != ECONNRESET) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                0,
                                "write error: %s",
                                strerror(errno)
                            );
                        }
                        Close(false);
                        owner->NetworkConnectionBroken();
                        break;
                    }
                } else if (amountSent > 0) {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        0,
                        "Sent %d bytes to %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16,
                        amountSent,
                        (uint8_t)((peerAddress >> 24) & 0xFF),
                        (uint8_t)((peerAddress >> 16) & 0xFF),
                        (uint8_t)((peerAddress >> 8) & 0xFF),
                        (uint8_t)(peerAddress & 0xFF),
                        peerPort
                    );
                    (void)platform->outputQueue.erase(platform->outputQueue.begin(), platform->outputQueue.begin() + amountSent);
                    if (
                        (amountSent == writeSize)
                        && !platform->outputQueue.empty()
                    ) {
                        wait = false;
                    }
                } else {
                    diagnosticsSender.SendDiagnosticInformationString(
                        0,
                        "zero bytes written"
                    );
                    Close(false);
                    owner->NetworkConnectionBroken();
                    break;
                }
            }
        }
        diagnosticsSender.SendDiagnosticInformationFormatted(
            0,
            "Processing stopped for connection with %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16,
            (uint8_t)((peerAddress >> 24) & 0xFF),
            (uint8_t)((peerAddress >> 16) & 0xFF),
            (uint8_t)((peerAddress >> 8) & 0xFF),
            (uint8_t)(peerAddress & 0xFF),
            peerPort
        );
    }

    void NetworkConnectionImpl::SendMessage(const std::vector< uint8_t >& message) {
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        platform->outputQueue.insert(platform->outputQueue.end(), message.begin(), message.end());
        platform->processorStateChangeSignal.Set();
    }

    void NetworkConnectionImpl::Close(bool stopProcessing) {
        if (
            stopProcessing
            && platform->processor.joinable()
        ) {
            platform->processorStop = true;
            platform->processorStateChangeSignal.Set();
            platform->processor.join();
        }
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        if (platform->sock >= 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0,
                "closing connection with %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16,
                (uint8_t)((peerAddress >> 24) & 0xFF),
                (uint8_t)((peerAddress >> 16) & 0xFF),
                (uint8_t)((peerAddress >> 8) & 0xFF),
                (uint8_t)(peerAddress & 0xFF),
                peerPort
            );
            (void)shutdown(platform->sock, SHUT_RDWR);
            (void)close(platform->sock);
            platform->sock = -1;
        }
    }

}
