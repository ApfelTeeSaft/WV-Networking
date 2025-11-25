#pragma once

#include <wvnet/Core.h>
#include <wvnet/NetDriver.h>
#include <wvnet/ReplicationManager.h>
#include <wvnet/RPCManager.h>

namespace WVNet {

    //=============================================================================
    // NetworkConfig - Configuration for network initialization
    //=============================================================================

    struct NetworkConfig {
        NetworkMode mode = NetworkMode::Standalone;
        std::string serverAddress = "127.0.0.1";
        uint16_t serverPort = DEFAULT_SERVER_PORT;
        uint32_t maxConnections = DEFAULT_MAX_CONNECTIONS;
        float tickRate = DEFAULT_TICK_RATE;
        bool enableRelevancy = false;
        float relevancyDistance = DEFAULT_RELEVANCY_DISTANCE;

        NetworkConfig() = default;
    };

    //=============================================================================
    // NetworkManager - Main networking subsystem manager (Singleton)
    //=============================================================================

    class NetworkManager {
    public:
        static NetworkManager& Get();

        // Initialization
        bool Initialize(const NetworkConfig& config);
        void Shutdown();
        bool IsInitialized() const { return m_initialized; }

        // Main update
        void Tick(float deltaTime);

        // Accessors
        NetworkMode GetMode() const { return m_config.mode; }
        bool IsServer() const { return m_config.mode == NetworkMode::Server; }
        bool IsClient() const { return m_config.mode == NetworkMode::Client; }
        bool IsNetworked() const { return m_config.mode != NetworkMode::Standalone; }

        NetDriver* GetNetDriver() { return m_netDriver.get(); }
        ReplicationManager* GetReplicationManager() { return m_replicationManager.get(); }
        RPCManager* GetRPCManager() { return m_rpcManager.get(); }

        const NetworkConfig& GetConfig() const { return m_config; }

        // Connection callbacks
        void OnClientConnected(NetConnection* connection);
        void OnClientDisconnected(NetConnection* connection);
        void OnPacketReceived(NetConnection* connection, const Packet& packet);

    private:
        NetworkManager();
        ~NetworkManager();

        // No copy
        NetworkManager(const NetworkManager&) = delete;
        NetworkManager& operator=(const NetworkManager&) = delete;

        bool m_initialized;
        NetworkConfig m_config;

        std::unique_ptr<NetDriver> m_netDriver;
        std::unique_ptr<ReplicationManager> m_replicationManager;
        std::unique_ptr<RPCManager> m_rpcManager;
    };

} // namespace WVNet
