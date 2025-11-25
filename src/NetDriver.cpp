#include <wvnet/NetDriver.h>

namespace WVNet {

    NetDriver::NetDriver()
        : m_mode(NetworkMode::Standalone)
        , m_maxConnections(DEFAULT_MAX_CONNECTIONS)
        , m_serverConnection(nullptr)
        , m_connectionTimeout(30.0f) {
    }

    NetDriver::~NetDriver() {
        Shutdown();
    }

    bool NetDriver::InitAsServer(uint16_t port, uint32_t maxConnections) {
        if (!SocketSystem::IsInitialized()) {
            WVNET_LOG_ERROR("Socket system not initialized");
            return false;
        }

        m_mode = NetworkMode::Server;
        m_maxConnections = maxConnections;

        // Create and bind socket
        if (!m_socket.CreateUDP()) {
            WVNET_LOG_ERROR("Failed to create server socket");
            return false;
        }

        if (!m_socket.SetNonBlocking(true)) {
            WVNET_LOG_ERROR("Failed to set socket to non-blocking");
            return false;
        }

        if (!m_socket.SetReuseAddress(true)) {
            WVNET_LOG_ERROR("Failed to set socket reuse address");
            return false;
        }

        if (!m_socket.Bind(port)) {
            WVNET_LOG_ERROR("Failed to bind server socket");
            return false;
        }

        WVNET_LOG_FMT("Server initialized on port %u", port);
        return true;
    }

    bool NetDriver::InitAsClient() {
        if (!SocketSystem::IsInitialized()) {
            WVNET_LOG_ERROR("Socket system not initialized");
            return false;
        }

        m_mode = NetworkMode::Client;

        // Create socket (don't bind to specific port)
        if (!m_socket.CreateUDP()) {
            WVNET_LOG_ERROR("Failed to create client socket");
            return false;
        }

        if (!m_socket.SetNonBlocking(true)) {
            WVNET_LOG_ERROR("Failed to set socket to non-blocking");
            return false;
        }

        // Bind to any available port
        if (!m_socket.Bind(0)) {
            WVNET_LOG_ERROR("Failed to bind client socket");
            return false;
        }

        WVNET_LOG("Client initialized");
        return true;
    }

    void NetDriver::Shutdown() {
        // Disconnect all clients
        for (auto& connection : m_connections) {
            if (connection->GetState() == ConnectionState::Connected) {
                Packet disconnectPacket(PacketType::Disconnect);
                SendPacket(connection.get(), disconnectPacket, false);
            }
        }

        m_connections.clear();
        m_connectionList.clear();
        m_serverConnection = nullptr;

        m_socket.Close();
        m_mode = NetworkMode::Standalone;

        WVNET_LOG("NetDriver shutdown");
    }

    bool NetDriver::ConnectToServer(const std::string& address, uint16_t port) {
        if (m_mode != NetworkMode::Client) {
            WVNET_LOG_ERROR("ConnectToServer can only be called in Client mode");
            return false;
        }

        WVSocketAddress serverAddr(address, port);
        if (!serverAddr.IsValid()) {
            WVNET_LOG_ERROR("Invalid server address");
            return false;
        }

        // Create connection to server
        m_serverConnection = CreateConnection(serverAddr);
        if (!m_serverConnection) {
            WVNET_LOG_ERROR("Failed to create server connection");
            return false;
        }

        // Send connection request
        Packet connectionRequest(PacketType::ConnectionRequest);
        SendPacket(m_serverConnection, connectionRequest, true);

        WVNET_LOG_FMT("Connecting to server %s...", serverAddr.ToString().c_str());
        return true;
    }

    void NetDriver::Tick(float deltaTime) {
        if (!IsInitialized()) {
            return;
        }

        // Receive incoming packets
        ReceivePackets();

        // Tick all connections
        for (auto& connection : m_connections) {
            connection->Tick(deltaTime);
        }

        // Flush outgoing packets
        FlushOutgoingPackets();

        // Check for timeouts
        CheckTimeouts();
    }

    void NetDriver::SendPacket(NetConnection* connection, const Packet& packet, bool reliable) {
        if (!connection) {
            return;
        }
        connection->SendPacket(packet, reliable);
    }

    void NetDriver::BroadcastPacket(const Packet& packet, bool reliable) {
        for (auto* connection : m_connectionList) {
            if (connection->GetState() == ConnectionState::Connected) {
                SendPacket(connection, packet, reliable);
            }
        }
    }

    NetConnection* NetDriver::FindConnection(const WVSocketAddress& address) const {
        for (const auto& connection : m_connections) {
            if (connection->GetAddress() == address) {
                return connection.get();
            }
        }
        return nullptr;
    }

    void NetDriver::DisconnectClient(NetConnection* connection) {
        if (!connection) {
            return;
        }

        Packet disconnectPacket(PacketType::Disconnect);
        SendPacket(connection, disconnectPacket, false);

        connection->SetState(ConnectionState::Disconnected);

        if (m_onDisconnection) {
            m_onDisconnection(connection);
        }

        RemoveConnection(connection);
    }

    void NetDriver::ReceivePackets() {
        constexpr size_t bufferSize = MAX_PACKET_SIZE * 2;
        uint8_t buffer[bufferSize];

        // Process up to 100 packets per tick to avoid starvation
        for (int i = 0; i < 100; ++i) {
            WVSocketAddress from;
            int32_t bytesReceived = m_socket.ReceiveFrom(buffer, bufferSize, from);

            if (bytesReceived <= 0) {
                break; // No more packets
            }

            // Deserialize packet
            BitStream stream(buffer, bytesReceived);
            Packet packet;
            if (!packet.Deserialize(stream)) {
                WVNET_LOG_ERROR("Failed to deserialize packet");
                continue;
            }

            // Find or create connection
            NetConnection* connection = FindConnection(from);

            // Handle connection request (server only)
            if (packet.GetType() == PacketType::ConnectionRequest) {
                if (IsServer()) {
                    HandleConnectionRequest(from, packet);
                }
                continue;
            }

            // Handle connection accept (client only)
            if (packet.GetType() == PacketType::ConnectionAccept) {
                if (IsClient() && m_serverConnection) {
                    m_serverConnection->SetState(ConnectionState::Connected);
                    WVNET_LOG("Connected to server");
                    if (m_onConnection) {
                        m_onConnection(m_serverConnection);
                    }
                }
                continue;
            }

            // Handle disconnect
            if (packet.GetType() == PacketType::Disconnect) {
                if (connection) {
                    HandleDisconnect(connection, packet);
                }
                continue;
            }

            // Process packet if connection exists
            if (connection) {
                connection->ReceivePacket(packet);

                // Notify packet callback
                if (m_onPacket) {
                    m_onPacket(connection, packet);
                }
            }
        }
    }

    void NetDriver::FlushOutgoingPackets() {
        for (auto& connection : m_connections) {
            connection->FlushOutgoing(&m_socket);
        }
    }

    void NetDriver::CheckTimeouts() {
        if (!IsServer()) {
            return;
        }

        // Check for timed out connections
        std::vector<NetConnection*> timedOut;
        for (auto* connection : m_connectionList) {
            if (connection->IsTimedOut(m_connectionTimeout)) {
                timedOut.push_back(connection);
            }
        }

        for (auto* connection : timedOut) {
            WVNET_LOG_FMT("Connection timed out: %s", connection->GetAddress().ToString().c_str());
            DisconnectClient(connection);
        }
    }

    void NetDriver::HandleConnectionRequest(const WVSocketAddress& from, const Packet& packet) {
        // Check if already connected
        if (FindConnection(from)) {
            WVNET_LOG_FMT("Connection request from already connected client: %s", from.ToString().c_str());
            return;
        }

        // Check max connections
        if (m_connections.size() >= m_maxConnections) {
            WVNET_LOG_FMT("Connection denied (max connections): %s", from.ToString().c_str());
            Packet deniedPacket(PacketType::ConnectionDenied);
            // Send directly without creating connection
            BitStream stream;
            deniedPacket.Serialize(stream);
            m_socket.SendTo(stream.GetData(), stream.GetSize(), from);
            return;
        }

        // Accept connection
        NetConnection* newConnection = CreateConnection(from);
        if (newConnection) {
            newConnection->SetState(ConnectionState::Connected);

            Packet acceptPacket(PacketType::ConnectionAccept);
            SendPacket(newConnection, acceptPacket, true);

            WVNET_LOG_FMT("Client connected: %s", from.ToString().c_str());

            if (m_onConnection) {
                m_onConnection(newConnection);
            }
        }
    }

    void NetDriver::HandleDisconnect(NetConnection* connection, const Packet& packet) {
        WVNET_LOG_FMT("Client disconnected: %s", connection->GetAddress().ToString().c_str());

        if (m_onDisconnection) {
            m_onDisconnection(connection);
        }

        RemoveConnection(connection);
    }

    NetConnection* NetDriver::CreateConnection(const WVSocketAddress& address) {
        auto connection = std::make_unique<NetConnection>(address);
        NetConnection* rawPtr = connection.get();

        m_connections.push_back(std::move(connection));
        m_connectionList.push_back(rawPtr);

        return rawPtr;
    }

    void NetDriver::RemoveConnection(NetConnection* connection) {
        // Remove from quick access list
        m_connectionList.erase(
            std::remove(m_connectionList.begin(), m_connectionList.end(), connection),
            m_connectionList.end()
        );

        // Remove from owned connections
        m_connections.erase(
            std::remove_if(m_connections.begin(), m_connections.end(),
                [connection](const std::unique_ptr<NetConnection>& conn) {
                    return conn.get() == connection;
                }),
            m_connections.end()
        );

        if (connection == m_serverConnection) {
            m_serverConnection = nullptr;
        }
    }

} // namespace WVNet
