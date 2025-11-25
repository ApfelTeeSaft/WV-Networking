#include <wvnet/NetworkManager.h>
#include <wvnet/World.h>

namespace WVNet {

    NetworkManager& NetworkManager::Get() {
        static NetworkManager instance;
        return instance;
    }

    NetworkManager::NetworkManager()
        : m_initialized(false) {
    }

    NetworkManager::~NetworkManager() {
        Shutdown();
    }

    bool NetworkManager::Initialize(const NetworkConfig& config) {
        if (m_initialized) {
            WVNET_LOG("NetworkManager already initialized");
            return true;
        }

        m_config = config;

        // Initialize socket system
        if (!SocketSystem::Initialize()) {
            WVNET_LOG_ERROR("Failed to initialize socket system");
            return false;
        }

        // Create subsystems
        m_netDriver = std::make_unique<NetDriver>();
        m_replicationManager = std::make_unique<ReplicationManager>();
        m_rpcManager = std::make_unique<RPCManager>();

        // Initialize replication manager
        m_replicationManager->Initialize(config.tickRate);
        m_replicationManager->SetRelevancyDistance(config.relevancyDistance);

        // Set up net driver callbacks
        m_netDriver->SetConnectionCallback([this](NetConnection* conn) {
            OnClientConnected(conn);
        });

        m_netDriver->SetDisconnectionCallback([this](NetConnection* conn) {
            OnClientDisconnected(conn);
        });

        m_netDriver->SetPacketCallback([this](NetConnection* conn, const Packet& packet) {
            OnPacketReceived(conn, packet);
        });

        // Initialize net driver based on mode
        bool success = false;
        if (config.mode == NetworkMode::Server) {
            success = m_netDriver->InitAsServer(config.serverPort, config.maxConnections);
            if (success) {
                WVNET_LOG_FMT("Server started on port %u", config.serverPort);
            }
        } else if (config.mode == NetworkMode::Client) {
            success = m_netDriver->InitAsClient();
            if (success) {
                success = m_netDriver->ConnectToServer(config.serverAddress, config.serverPort);
                if (success) {
                    WVNET_LOG_FMT("Connecting to server %s:%u", config.serverAddress.c_str(), config.serverPort);
                }
            }
        } else {
            WVNET_LOG("NetworkManager initialized in Standalone mode");
            success = true;
        }

        if (!success) {
            WVNET_LOG_ERROR("Failed to initialize NetDriver");
            Shutdown();
            return false;
        }

        m_initialized = true;
        WVNET_LOG("NetworkManager initialized successfully");
        return true;
    }

    void NetworkManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        WVNET_LOG("Shutting down NetworkManager");

        if (m_netDriver) {
            m_netDriver->Shutdown();
        }

        m_netDriver.reset();
        m_replicationManager.reset();
        m_rpcManager.reset();

        SocketSystem::Shutdown();

        m_initialized = false;
    }

    void NetworkManager::Tick(float deltaTime) {
        if (!m_initialized || !IsNetworked()) {
            return;
        }

        // Tick net driver (send/receive packets)
        if (m_netDriver) {
            m_netDriver->Tick(deltaTime);
        }

        // Tick replication manager (replicate actors to clients)
        if (m_replicationManager && IsServer()) {
            // Register all replicated actors from the world
            for (Actor* actor : World::Get().GetActors()) {
                if (actor->GetReplicates()) {
                    m_replicationManager->RegisterActor(actor);
                }
            }

            m_replicationManager->Tick(deltaTime, m_netDriver.get());
        }
    }

    void NetworkManager::OnClientConnected(NetConnection* connection) {
        if (!connection) {
            return;
        }

        WVNET_LOG_FMT("Client connected: %s", connection->GetAddress().ToString().c_str());

        // Server: spawn existing actors for new client
        if (IsServer()) {
            for (Actor* actor : World::Get().GetActors()) {
                if (actor->GetReplicates()) {
                    m_replicationManager->RegisterActor(actor);
                }
            }
        }
    }

    void NetworkManager::OnClientDisconnected(NetConnection* connection) {
        if (!connection) {
            return;
        }

        WVNET_LOG_FMT("Client disconnected: %s", connection->GetAddress().ToString().c_str());

        // TODO: Clean up any client-specific data
    }

    void NetworkManager::OnPacketReceived(NetConnection* connection, const Packet& packet) {
        PacketType type = packet.GetType();

        // Route packets to appropriate managers
        switch (type) {
            case PacketType::ActorSpawn:
            case PacketType::ActorDestroy:
            case PacketType::ActorReplication:
                if (m_replicationManager) {
                    m_replicationManager->ProcessActorReplication(connection, packet);
                }
                break;

            case PacketType::RPCServer:
            case PacketType::RPCClient:
            case PacketType::RPCMulticast:
                if (m_rpcManager) {
                    m_rpcManager->ProcessRPC(connection, packet, m_netDriver.get());
                }
                break;

            case PacketType::Heartbeat:
                // Heartbeat handled by NetConnection
                break;

            default:
                WVNET_LOG_FMT("Unhandled packet type: %d", static_cast<int>(type));
                break;
        }
    }

} // namespace WVNet
