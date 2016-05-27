/**
 * @file NetworkConnectionWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::NetworkConnectionImpl class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../NetworkConnectionImpl.hpp"
#include "NetworkConnectionWin32.hpp"

#include <algorithm>
#include <inttypes.h>
#include <WinSock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32")
#undef ERROR
#undef SendMessage
#undef min

namespace {

    static const size_t MAXIMUM_READ_SIZE = 65536;
    static const size_t MAXIMUM_WRITE_SIZE = 65536;

}

namespace SystemAbstractions {

    bool NetworkConnectionPlatform::Bind(
        SOCKET& sock,
        uint16_t& port,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender
    ) {
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_port = htons(port);
        sock = socket(socketAddress.sin_family, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error creating socket (%d)",
                WSAGetLastError()
            );
            return false;
        }
        if (bind(sock, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error in bind (%d)",
                WSAGetLastError()
            );
            (void)closesocket(sock);
            sock = INVALID_SOCKET;
            return false;
        }
        int socketAddressLength = sizeof(socketAddress);
        if (getsockname(sock, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0) {
            port = ntohs(socketAddress.sin_port);
        } else {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error in getsockname (%d)",
                WSAGetLastError()
            );
            (void)closesocket(sock);
            sock = INVALID_SOCKET;
            return false;
        }
        return true;
    }

    NetworkConnectionImpl::NetworkConnectionImpl()
        : platform(new NetworkConnectionPlatform())
        , diagnosticsSender("NetworkConnection")
    {
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
            platform->wsaStarted = true;
        }
    }

    NetworkConnectionImpl::~NetworkConnectionImpl() {
        Close(true);
        if (platform->wsaStarted) {
            (void)WSACleanup();
        }
        if (platform->socketEvent != NULL) {
            (void)CloseHandle(platform->socketEvent);
        }
        if (platform->processorStateChangeEvent != NULL) {
            (void)CloseHandle(platform->processorStateChangeEvent);
        }
    }

    bool NetworkConnectionImpl::Connect() {
        Close(true);
        uint16_t port = 0;
        if (!NetworkConnectionPlatform::Bind(platform->sock, port, diagnosticsSender)) {
            return false;
        }
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.S_un.S_addr = htonl(peerAddress);
        socketAddress.sin_port = htons(peerPort);
        if (connect(platform->sock, (const sockaddr*)&socketAddress, sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error in connect (%d)",
                WSAGetLastError()
            );
            Close(false);
            return false;
        }
        return true;
    }

    bool NetworkConnectionImpl::Process() {
        if (platform->sock == INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "not connected"
            );
            return false;
        }
        if (platform->processor.joinable()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsReceiver::Levels::WARNING,
                "already processing"
            );
            return true;
        }
        platform->processorStop = false;
        if (platform->processorStateChangeEvent == NULL) {
            platform->processorStateChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->processorStateChangeEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                    "error creating processor state change event (%d)",
                    (int)GetLastError()
                );
                return false;
            }
        }
        if (platform->socketEvent == NULL) {
            platform->socketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->socketEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                    "error creating socket event (%d)",
                    (int)GetLastError()
                );
                return false;
            }
        }
        if (WSAEventSelect(platform->sock, platform->socketEvent, FD_READ | FD_WRITE | FD_CLOSE) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsReceiver::Levels::ERROR,
                "error in WSAEventSelect for FD_READ (%d)",
                WSAGetLastError()
            );
            return false;
        }
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
        const HANDLE handles[2] = { platform->processorStateChangeEvent, platform->socketEvent };
        std::vector< uint8_t > buffer;
        DWORD eventIndex = 0;
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        bool wait = true;
        while (!platform->processorStop) {
            if (wait) {
                processingLock.unlock();
                const auto eventIndex = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                processingLock.lock();
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    0,
                    "processor awakened by event, index %d",
                    (int)eventIndex
                );
            }
            wait = true;
            buffer.resize(MAXIMUM_READ_SIZE);
            const int amountReceived = recv(platform->sock, (char*)&buffer[0], (int)buffer.size(), 0);
            if (amountReceived == SOCKET_ERROR) {
                const auto wsaLastError = WSAGetLastError();
                if (wsaLastError == WSAEWOULDBLOCK) {
                    diagnosticsSender.SendDiagnosticInformationString(
                        0,
                        "read not ready"
                    );
                } else {
                    if (wsaLastError != WSAECONNRESET) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            0,
                            "read error %d",
                            wsaLastError
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
                const int amountSent = send(platform->sock, (const char*)&buffer[0], writeSize, 0);
                if (amountSent == SOCKET_ERROR) {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError == WSAEWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationString(
                            0,
                            "write not ready"
                        );
                    } else {
                        if (wsaLastError != WSAECONNRESET) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                0,
                                "write error %d",
                                wsaLastError
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
        (void)SetEvent(platform->processorStateChangeEvent);
    }

    void NetworkConnectionImpl::Close(bool stopProcessing) {
        if (
            stopProcessing
            && platform->processor.joinable()
        ) {
            platform->processorStop = true;
            (void)SetEvent(platform->processorStateChangeEvent);
            platform->processor.join();
        }
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        if (platform->sock != INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0,
                "closing connection with %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16,
                (uint8_t)((peerAddress >> 24) & 0xFF),
                (uint8_t)((peerAddress >> 16) & 0xFF),
                (uint8_t)((peerAddress >> 8) & 0xFF),
                (uint8_t)(peerAddress & 0xFF),
                peerPort
            );
            (void)closesocket(platform->sock);
            platform->sock = INVALID_SOCKET;
        }
    }

}
