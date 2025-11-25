#pragma once

#include <wvnet/Core.h>
#include <wvnet/platform/Socket.h>
#include <wvnet/NetConnection.h>
#include <wvnet/Packet.h>
#include <vector>
#include <memory>
#include <functional>

namespace WVNet {

    //=============================================================================
    // NetDriver - Low-level network driver managing connections and sockets
    //=============================================================================

    class NetDriver {
    public:
        using ConnectionCallback = std::function<void(NetConnection*)>;
        using DisconnectionCallback = std::function<void(NetConnection*)>;
        using PacketCallback = std::function<void(NetConnection*, const Packet&)>;

        NetDriver();
        ~NetDriver();

        // Initialization
        bool InitAsServer(uint16_t port, uint32_t maxConnections);
        bool InitAsClient();
        void Shutdown();

        // Client operations
        bool ConnectToServer(const std::string& address, uint16_t port);

        // Tick
        void Tick(float deltaTime);

        // Sending
        void SendPacket(NetConnection* connection, const Packet& packet, bool reliable = true);
        void BroadcastPacket(const Packet& packet, bool reliable = true);

        // Connection management
        const std::vector<NetConnection*>& GetConnections() const { return m_connectionList; }
        NetConnection* GetServerConnection() const { return m_serverConnection; } // For clients
        NetConnection* FindConnection(const WVSocketAddress& address) const;
        void DisconnectClient(NetConnection* connection);

        // Callbacks
        void SetConnectionCallback(ConnectionCallback callback) { m_onConnection = callback; }
        void SetDisconnectionCallback(DisconnectionCallback callback) { m_onDisconnection = callback; }
        void SetPacketCallback(PacketCallback callback) { m_onPacket = callback; }

        // State
        NetworkMode GetMode() const { return m_mode; }
        bool IsServer() const { return m_mode == NetworkMode::Server; }
        bool IsClient() const { return m_mode == NetworkMode::Client; }
        bool IsInitialized() const { return m_socket.IsValid(); }

    private:
        void ReceivePackets();
        void FlushOutgoingPackets();
        void CheckTimeouts();

        void HandleConnectionRequest(const WVSocketAddress& from, const Packet& packet);
        void HandleDisconnect(NetConnection* connection, const Packet& packet);

        NetConnection* CreateConnection(const WVSocketAddress& address);
        void RemoveConnection(NetConnection* connection);

        NetworkMode m_mode;
        WVSocket m_socket;
        uint32_t m_maxConnections;

        // Connections
        std::vector<std::unique_ptr<NetConnection>> m_connections;
        std::vector<NetConnection*> m_connectionList; // Raw pointers for quick access
        NetConnection* m_serverConnection; // For clients only

        // Callbacks
        ConnectionCallback m_onConnection;
        DisconnectionCallback m_onDisconnection;
        PacketCallback m_onPacket;

        // Timing
        float m_connectionTimeout;
    };

} // namespace WVNet
