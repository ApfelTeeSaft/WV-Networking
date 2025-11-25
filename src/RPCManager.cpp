#include <wvnet/RPCManager.h>
#include <wvnet/NetDriver.h>
#include <wvnet/World.h>
#include <wvnet/NetworkManager.h>

namespace WVNet {

    RPCManager::RPCManager() {
    }

    RPCManager::~RPCManager() {
    }

    void RPCManager::RegisterRPC(const std::string& functionName, RPCType type, RPCHandler handler) {
        RPCMetadata metadata(functionName, type, handler);
        m_rpcRegistry[functionName] = metadata;
        WVNET_LOG_FMT("Registered RPC: %s (type: %d)", functionName.c_str(), static_cast<int>(type));
    }

    void RPCManager::CallServerRPC(Actor* actor, const std::string& functionName, BitStream& params) {
        if (!actor) {
            WVNET_LOG_ERROR("CallServerRPC: actor is null");
            return;
        }

        auto& netMgr = NetworkManager::Get();
        if (!netMgr.IsClient()) {
            WVNET_LOG_ERROR("CallServerRPC can only be called from client");
            return;
        }

        NetDriver* netDriver = netMgr.GetNetDriver();
        NetConnection* serverConnection = netDriver ? netDriver->GetServerConnection() : nullptr;

        if (!serverConnection) {
            WVNET_LOG_ERROR("CallServerRPC: not connected to server");
            return;
        }

        SendRPC(serverConnection, PacketType::RPCServer, actor->GetNetId(), functionName, params, netDriver);
    }

    void RPCManager::CallClientRPC(Actor* actor, NetConnection* client, const std::string& functionName, BitStream& params) {
        if (!actor || !client) {
            WVNET_LOG_ERROR("CallClientRPC: actor or client is null");
            return;
        }

        auto& netMgr = NetworkManager::Get();
        if (!netMgr.IsServer()) {
            WVNET_LOG_ERROR("CallClientRPC can only be called from server");
            return;
        }

        NetDriver* netDriver = netMgr.GetNetDriver();
        if (!netDriver) {
            WVNET_LOG_ERROR("CallClientRPC: NetDriver is null");
            return;
        }

        SendRPC(client, PacketType::RPCClient, actor->GetNetId(), functionName, params, netDriver);
    }

    void RPCManager::CallMulticastRPC(Actor* actor, const std::string& functionName, BitStream& params) {
        if (!actor) {
            WVNET_LOG_ERROR("CallMulticastRPC: actor is null");
            return;
        }

        auto& netMgr = NetworkManager::Get();
        if (!netMgr.IsServer()) {
            WVNET_LOG_ERROR("CallMulticastRPC can only be called from server");
            return;
        }

        NetDriver* netDriver = netMgr.GetNetDriver();
        if (!netDriver) {
            WVNET_LOG_ERROR("CallMulticastRPC: NetDriver is null");
            return;
        }

        // Send to all connected clients
        for (NetConnection* connection : netDriver->GetConnections()) {
            if (connection->GetState() == ConnectionState::Connected) {
                SendRPC(connection, PacketType::RPCMulticast, actor->GetNetId(), functionName, params, netDriver);
            }
        }
    }

    void RPCManager::ProcessRPC(NetConnection* connection, const Packet& packet, NetDriver* netDriver) {
        // Read RPC data
        uint32_t actorNetId = packet.GetPayload().ReadUInt32();
        std::string functionName = packet.GetPayload().ReadString();

        // Find actor
        Actor* actor = World::Get().GetActorByNetId(actorNetId);
        if (!actor) {
            WVNET_LOG_FMT("ProcessRPC: Actor not found (NetID: %u)", actorNetId);
            return;
        }

        // Find RPC handler
        auto it = m_rpcRegistry.find(functionName);
        if (it == m_rpcRegistry.end()) {
            WVNET_LOG_FMT("ProcessRPC: RPC not registered: %s", functionName.c_str());
            return;
        }

        const RPCMetadata& metadata = it->second;

        // Verify RPC type matches packet type
        PacketType expectedPacketType = PacketType::RPCServer;
        switch (metadata.type) {
            case RPCType::Server:
                expectedPacketType = PacketType::RPCServer;
                break;
            case RPCType::Client:
                expectedPacketType = PacketType::RPCClient;
                break;
            case RPCType::Multicast:
                expectedPacketType = PacketType::RPCMulticast;
                break;
        }

        if (packet.GetType() != expectedPacketType) {
            WVNET_LOG_ERROR("ProcessRPC: Packet type mismatch");
            return;
        }

        // Execute RPC handler with remaining parameters
        BitStream params(packet.GetPayload().GetData() + packet.GetPayload().GetReadPos(),
                        packet.GetPayload().GetBytesRemaining());
        metadata.handler(actor, params);
    }

    void RPCManager::SendRPC(NetConnection* connection, PacketType packetType, uint32_t actorNetId,
                            const std::string& functionName, BitStream& params, NetDriver* netDriver) {
        Packet packet(packetType);

        // Write RPC data
        packet.Write(actorNetId);
        packet.GetPayload().WriteString(functionName);

        // Append parameters
        if (params.GetSize() > 0) {
            packet.GetPayload().Write(params.GetData(), params.GetSize());
        }

        netDriver->SendPacket(connection, packet, true);
    }

} // namespace WVNet
