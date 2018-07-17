/**
 * @file NetworkEndpointTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::NetworkEndpoint class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <SystemAbstractions/NetworkEndpoint.hpp>

#ifdef _WIN32
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
#else /* POSIX */
#include <sys/socket.h>
#endif /* _WIN32 or POSIX */

namespace {

    /**
     * This is used to hold all the information about a received
     * datagram that we care about.
     */
    struct Packet {
        /**
         * This is a copy of the data from the received datagram.
         */
        std::vector< uint8_t > payload;

        /**
         * This is the IPv4 address of the datagram sender.
         */
        uint32_t address;

        /**
         * This is the port number of the datagram sender.
         */
        uint16_t port;

        /**
         * This constructor initializes all properies of the structure.
         *
         * @param[in] newPayload
         *     This is a copy of the data from the received datagram.
         *
         * @param[in] newAddress
         *     This is the IPv4 address of the datagram sender.
         *
         * @param[in] port
         *     This is the port number of the datagram sender.
         */
        Packet(
            const std::vector< uint8_t >& newPayload,
            uint32_t newAddress,
            uint16_t newPort
        )
            : payload(newPayload)
            , address(newAddress)
            , port(newPort)
        {
        }
    };

    /**
     * This is used to receive callbacks from the unit under test.
     */
    struct Owner {
        // Properties

        /**
         * This is used to wait for, or signal, a condition
         * upon which that the owner might be waiting.
         */
        std::condition_variable_any condition;

        /**
         * This is used to synchronize access to the class.
         */
        std::mutex mutex;

        /**
         * This holds a copy of each packet received.
         */
        std::vector< Packet > packetsReceived;

        /**
         * This holds the data received from a connection-oriented stream.
         */
        std::vector< uint8_t > streamReceived;

        /**
         * These are connections that have been established
         * between the unit under test and remote clients.
         */
        std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > connections;

        /**
         * This flag indicates whether or not a connection
         * to the network endpoint has been broken.
         */
        bool connectionBroken = false;

        // Methods

        /**
         * This method waits up to a second for a packet
         * to be received from the network endpoint.
         *
         * @return
         *     An indication of whether or not a packet
         *     is received from the network endpoint
         *     is returned.
         */
        bool AwaitPacket() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !packetsReceived.empty();
                }
            );
        }

        /**
         * This method waits up to a second for the given number
         * of bytes to be received from a client connected
         * to the network endpoint.
         *
         * @param[in] numBytes
         *     This is the number of bytes we expect to receive.
         *
         * @return
         *     An indication of whether or not the given number
         *     of bytes were received from a client connected
         *     to the network endpoint is returned.
         */
        bool AwaitStream(size_t numBytes) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this, numBytes]{
                    return (streamReceived.size() >= numBytes);
                }
            );
        }

        /**
         * This method waits up to a second for a connection
         * to be received at the network endpoint.
         *
         * @return
         *     An indication of whether or not a connection
         *     is received at the network endpoint
         *     is returned.
         */
        bool AwaitConnection() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !connections.empty();
                }
            );
        }

        /**
         * This method waits up to a second for the given number
         * of connections to be received at the network endpoint.
         *
         * @param[in] numConnections
         *     This is the number of connections for which to wait.
         *
         * @return
         *     An indication of whether or not the given number
         *     of connections is received at the network endpoint
         *     is returned.
         */
        bool AwaitConnections(size_t numConnections) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this, numConnections]{
                    return (connections.size() >= numConnections);
                }
            );
        }

        /**
         * This method waits up to a second for a connection
         * to the network endpoint to be broken.
         *
         * @return
         *     An indication of whether or not a connection
         *     to the network endpoint is broken
         *     is returned.
         */
        bool AwaitDisconnection() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return connectionBroken;
                }
            );
        }

        /**
         * This is the callback function to be called whenever
         * a new client connects to the network endpoint.
         *
         * @param[in] newConnection
         *     This represents the connection to the new client.
         */
        void NetworkEndpointNewConnection(std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            connections.push_back(newConnection);
            condition.notify_all();
            (void)newConnection->Process(
                [this](const std::vector< uint8_t >& message){
                    NetworkConnectionMessageReceived(message);
                },
                [this]{
                    NetworkConnectionBroken();
                }
            );
        }

        /**
         * This is the type of callback function to be called whenever
         * a new datagram-oriented message is received by the network endpoint.
         *
         * @param[in] address
         *     This is the IPv4 address of the client who sent the message.
         *
         * @param[in] port
         *     This is the port number of the client who sent the message.
         *
         * @param[in] body
         *     This is the contents of the datagram sent by the client.
         */
        void NetworkEndpointPacketReceived(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            packetsReceived.emplace_back(body, address, port);
            condition.notify_all();
        }

        /**
         * This is the callback issued whenever more data
         * is received from the peer of the connection.
         *
         * @param[in] message
         *     This contains the data received from
         *     the peer of the connection.
         */
        void NetworkConnectionMessageReceived(const std::vector< uint8_t >& message) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            streamReceived.insert(
                streamReceived.end(),
                message.begin(),
                message.end()
            );
            condition.notify_all();
        }

        /**
         * This is the callback issued whenever
         * the connection is broken.
         */
        void NetworkConnectionBroken() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            connectionBroken = true;
            condition.notify_all();
        }
    };

}

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct NetworkEndpointTests
    : public ::testing::Test
{
    // Properties

    /**
     * This keeps track of whether or not WSAStartup succeeded,
     * because if so we need to call WSACleanup upon teardown.
     */
    bool wsaStarted = false;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
#if _WIN32
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
            wsaStarted = true;
        }
#endif /* _WIN32 */
    }

    virtual void TearDown() {
#if _WIN32
        if (wsaStarted) {
            (void)WSACleanup();
        }
#endif /* _WIN32 */
    }
};

TEST_F(NetworkEndpointTests, DatagramSending) {
    // Set up a datagram socket to test sending from NetworkEndpoint.
    auto receiver = socket(AF_INET, SOCK_DGRAM, 0);
#if _WIN32
    ASSERT_FALSE(receiver == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(receiver < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = 0;
    receiverAddress.sin_port = 0;
    ASSERT_TRUE(bind(receiver, (struct sockaddr*)&receiverAddress, sizeof(receiverAddress)) == 0);
    int receiverAddressLength = sizeof(receiverAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&receiverAddress, &receiverAddressLength) == 0);
    port = ntohs(receiverAddress.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ){ owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){ owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Datagram,
        0,
        0,
        0
    );

    // Test sending a datagram from the unit under test.
    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    endpoint.SendPacket(0x7F000001, port, testPacket);

    // Verify that we received the datagram.
    struct sockaddr_in senderAddress;
    int senderAddressSize = sizeof(senderAddress);
    std::vector< uint8_t > buffer(testPacket.size() * 2);
    const int amountReceived = recvfrom(
        receiver,
        (char*)buffer.data(),
        (int)buffer.size(),
        0,
        (struct sockaddr*)&senderAddress,
        &senderAddressSize
    );
    ASSERT_EQ(testPacket.size(), amountReceived);
    buffer.resize(amountReceived);
    ASSERT_EQ(testPacket, buffer);
    ASSERT_EQ(0x7F000001, ntohl(senderAddress.sin_addr.S_un.S_addr));
    ASSERT_EQ(endpoint.GetBoundPort(), ntohs(senderAddress.sin_port));
}

TEST_F(NetworkEndpointTests, DatagramReceiving) {
    // Set up a datagram socket to test sending to NetworkEndpoint.
    auto sender = socket(AF_INET, SOCK_DGRAM, 0);
#if _WIN32
    ASSERT_FALSE(sender == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(sender < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.sin_addr.S_un.S_addr = 0;
    senderAddress.sin_port = 0;
    ASSERT_TRUE(bind(sender, (struct sockaddr*)&senderAddress, sizeof(senderAddress)) == 0);
    int senderAddressLength = sizeof(senderAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(sender, (struct sockaddr*)&senderAddress, &senderAddressLength) == 0);
    port = ntohs(senderAddress.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ){ owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){ owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Datagram,
        0,
        0,
        0
    );

    // Test receiving a datagram at the unit under test.
    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endpoint.GetBoundPort());
    (void)sendto(
        sender,
        (const char*)testPacket.data(),
        (int)testPacket.size(),
        0,
        (const sockaddr*)&receiverAddress,
        sizeof(receiverAddress)
    );

    // Verify that we received the datagram.
    ASSERT_TRUE(owner.AwaitPacket());
    ASSERT_EQ(testPacket, owner.packetsReceived[0].payload);
    ASSERT_EQ(0x7F000001, owner.packetsReceived[0].address);
    ASSERT_EQ(ntohs(senderAddress.sin_port), owner.packetsReceived[0].port);
}

TEST_F(NetworkEndpointTests, ConnectionSending) {
    // Set up a connection-oriented socket to test sending
    // from NetworkEndpoint.
    auto receiver = socket(AF_INET, SOCK_STREAM, 0);
#if _WIN32
    ASSERT_FALSE(receiver == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(receiver < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = 0;
    receiverAddress.sin_port = 0;
    ASSERT_TRUE(bind(receiver, (struct sockaddr*)&receiverAddress, sizeof(receiverAddress)) == 0);
    int receiverAddressLength = sizeof(receiverAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&receiverAddress, &receiverAddressLength) == 0);
    port = ntohs(receiverAddress.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ){ owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){ owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint.
    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    senderAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            receiver,
            (const sockaddr*)&senderAddress,
            sizeof(senderAddress)
        ) == 0
    );
    owner.AwaitConnection();
    struct sockaddr_in socketAddress;
    int socketAddressLength = sizeof(socketAddress);
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0);
    ASSERT_EQ(ntohl(senderAddress.sin_addr.S_un.S_addr), owner.connections[0]->GetBoundAddress());
    ASSERT_EQ(ntohs(senderAddress.sin_port), owner.connections[0]->GetBoundPort());

    // Test sending a message from the unit under test.
    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    owner.connections[0]->SendMessage(testPacket);

    // Verify that we received the message from the unit under test.
    std::vector< uint8_t > buffer(testPacket.size());
    const int amountReceived = recv(
        receiver,
        (char*)buffer.data(),
        (int)buffer.size(),
        0
    );
    ASSERT_EQ(testPacket.size(), amountReceived);
    ASSERT_EQ(testPacket, buffer);
}

TEST_F(NetworkEndpointTests, ConnectionReceiving) {
    // Set up a connection-oriented socket to test sending
    // to NetworkEndpoint.
    auto sender = socket(AF_INET, SOCK_STREAM, 0);
#if _WIN32
    ASSERT_FALSE(sender == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(sender < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.sin_addr.S_un.S_addr = 0;
    senderAddress.sin_port = 0;
    ASSERT_TRUE(bind(sender, (struct sockaddr*)&senderAddress, sizeof(senderAddress)) == 0);
    int senderAddressLength = sizeof(senderAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(sender, (struct sockaddr*)&senderAddress, &senderAddressLength) == 0);
    port = ntohs(senderAddress.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ){ owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){ owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint.
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            sender,
            (const sockaddr*)&receiverAddress,
            sizeof(receiverAddress)
        ) == 0
    );
    owner.AwaitConnection();

    // Test receiving a message at the unit under test.
    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    (void)send(
        sender,
        (char*)testPacket.data(),
        (int)testPacket.size(),
        0
    );

    // Verify that we received the message at the unit under test.
    owner.AwaitStream(testPacket.size());
    ASSERT_EQ(testPacket, owner.streamReceived);
}

TEST_F(NetworkEndpointTests, ConnectionBroken) {
    // Set up a connection-oriented socket to test sending
    // from NetworkEndpoint.
    auto client = socket(AF_INET, SOCK_STREAM, 0);
#if _WIN32
    ASSERT_FALSE(client == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(client < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in clientAddress;
    (void)memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.S_un.S_addr = 0;
    clientAddress.sin_port = 0;
    ASSERT_TRUE(bind(client, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) == 0);
    int clientAddressLength = sizeof(clientAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(client, (struct sockaddr*)&clientAddress, &clientAddressLength) == 0);
    port = ntohs(clientAddress.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ){ owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){ owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint.
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    serverAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            client,
            (const sockaddr*)&serverAddress,
            sizeof(serverAddress)
        ) == 0
    );
    owner.AwaitConnection();

    // Break the connection from the client, and verify
    // that the broken connection is detected at the network endpoint.
#ifdef _WIN32
    (void)closesocket(client);
#else /* POSIX */
    (void)close(client);
#endif /* _WIN32 / POSIX */
    owner.AwaitDisconnection();
}

TEST_F(NetworkEndpointTests, MultipleConnections) {
    // Set up two connection-oriented sockets to test sending
    // to NetworkEndpoint.
    auto sender1 = socket(AF_INET, SOCK_STREAM, 0);
    auto sender2 = socket(AF_INET, SOCK_STREAM, 0);
#if _WIN32
    ASSERT_FALSE(sender1 == INVALID_SOCKET);
    ASSERT_FALSE(sender2 == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(sender1 < 0);
    ASSERT_FALSE(sender2 < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in sender1Address;
    (void)memset(&sender1Address, 0, sizeof(sender1Address));
    sender1Address.sin_family = AF_INET;
    sender1Address.sin_addr.S_un.S_addr = 0;
    sender1Address.sin_port = 0;
    struct sockaddr_in sender2Address = sender1Address;
    ASSERT_TRUE(bind(sender1, (struct sockaddr*)&sender1Address, sizeof(sender1Address)) == 0);
    ASSERT_TRUE(bind(sender2, (struct sockaddr*)&sender2Address, sizeof(sender2Address)) == 0);
    int sender1AddressLength = sizeof(sender1Address);
    int sender2AddressLength = sizeof(sender2Address);
    ASSERT_TRUE(getsockname(sender1, (struct sockaddr*)&sender1Address, &sender1AddressLength) == 0);
    ASSERT_TRUE(getsockname(sender2, (struct sockaddr*)&sender2Address, &sender2AddressLength) == 0);
    const auto port1 = ntohs(sender1Address.sin_port);
    const auto port2 = ntohs(sender2Address.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ){ owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){ owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint.
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            sender1,
            (const sockaddr*)&receiverAddress,
            sizeof(receiverAddress)
        ) == 0
    );
    ASSERT_TRUE(
        connect(
            sender2,
            (const sockaddr*)&receiverAddress,
            sizeof(receiverAddress)
        ) == 0
    );
    owner.AwaitConnections(2);

    // Test receiving a message at the unit under test.
    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    (void)send(
        sender1,
        (char*)testPacket.data(),
        (int)testPacket.size() / 2,
        0
    );
    owner.AwaitStream(testPacket.size() / 2);
    (void)send(
        sender2,
        (char*)testPacket.data() + testPacket.size() / 2,
        (int)(testPacket.size() - testPacket.size() / 2),
        0
    );

    // Verify that we received the message at the unit under test.
    owner.AwaitStream(testPacket.size());
    ASSERT_EQ(testPacket, owner.streamReceived);
}
