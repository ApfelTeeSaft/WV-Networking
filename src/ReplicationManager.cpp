#include <wvnet/ReplicationManager.h>
#include <wvnet/NetDriver.h>
#include <wvnet/World.h>
#include <wvnet/NetworkManager.h>
#include <algorithm>

namespace WVNet {

    ReplicationManager::ReplicationManager()
        : m_tickRate(DEFAULT_TICK_RATE)
        , m_replicationInterval(1.0f / DEFAULT_TICK_RATE)
        , m_timeSinceLastReplication(0.0f)
        , m_relevancyDistance(DEFAULT_RELEVANCY_DISTANCE) {
    }

    ReplicationManager::~ReplicationManager() {
    }

    void ReplicationManager::Initialize(float tickRate) {
        SetTickRate(tickRate);
    }

    void ReplicationManager::Tick(float deltaTime, NetDriver* netDriver) {
        if (!netDriver || !netDriver->IsServer()) {
            return;
        }

        m_timeSinceLastReplication += deltaTime;

        if (m_timeSinceLastReplication >= m_replicationInterval) {
            // Replicate to all connected clients
            for (auto* connection : netDriver->GetConnections()) {
                if (connection->GetState() == ConnectionState::Connected) {
                    ReplicateActors(connection, netDriver);
                }
            }

            m_timeSinceLastReplication = 0.0f;
        }
    }

    void ReplicationManager::RegisterActor(Actor* actor) {
        if (!actor || !actor->GetReplicates()) {
            return;
        }

        // Add to replicated actors list if not already present
        if (std::find(m_replicatedActors.begin(), m_replicatedActors.end(), actor) == m_replicatedActors.end()) {
            m_replicatedActors.push_back(actor);
        }
    }

    void ReplicationManager::UnregisterActor(Actor* actor) {
        if (!actor) {
            return;
        }

        // Remove from replicated actors list
        m_replicatedActors.erase(
            std::remove(m_replicatedActors.begin(), m_replicatedActors.end(), actor),
            m_replicatedActors.end()
        );

        // TODO: Send destroy packets to clients
    }

    void ReplicationManager::ReplicateActors(NetConnection* connection, NetDriver* netDriver) {
        if (!connection || !netDriver) {
            return;
        }

        for (Actor* actor : m_replicatedActors) {
            if (!IsActorRelevantForConnection(actor, connection)) {
                continue;
            }

            ActorReplicationState* state = GetOrCreateReplicationState(connection, actor->GetNetId());

            // First time replicating this actor to this client?
            if (!state->spawned) {
                SendActorSpawn(actor, connection, netDriver);
                state->spawned = true;
            }

            // Send property updates
            SendActorUpdate(actor, connection, netDriver);
        }
    }

    void ReplicationManager::ProcessActorReplication(NetConnection* connection, const Packet& packet) {
        PacketType type = packet.GetType();

        switch (type) {
            case PacketType::ActorSpawn:
                HandleActorSpawn(connection, packet);
                break;
            case PacketType::ActorDestroy:
                HandleActorDestroy(connection, packet);
                break;
            case PacketType::ActorReplication:
                HandleActorUpdate(connection, packet);
                break;
            default:
                break;
        }
    }

    bool ReplicationManager::IsActorRelevantForConnection(Actor* actor, NetConnection* connection) {
        // Simple implementation: all actors are relevant
        // TODO: Implement distance-based or custom relevancy
        return true;
    }

    void ReplicationManager::SetRelevancyDistance(float distance) {
        m_relevancyDistance = distance;
    }

    void ReplicationManager::SetTickRate(float tickRate) {
        m_tickRate = tickRate;
        m_replicationInterval = 1.0f / tickRate;
    }

    void ReplicationManager::SendActorSpawn(Actor* actor, NetConnection* connection, NetDriver* netDriver) {
        Packet packet(PacketType::ActorSpawn);

        // Serialize actor data
        packet.Write(actor->GetNetId());
        packet.GetPayload().WriteString(actor->GetTypeName());
        packet.GetPayload().WriteVector3(actor->GetPosition());
        packet.GetPayload().WriteQuaternion(actor->GetRotation());

        netDriver->SendPacket(connection, packet, true);
    }

    void ReplicationManager::SendActorDestroy(uint32_t actorNetId, NetConnection* connection, NetDriver* netDriver) {
        Packet packet(PacketType::ActorDestroy);
        packet.Write(actorNetId);
        netDriver->SendPacket(connection, packet, true);
    }

    void ReplicationManager::SendActorUpdate(Actor* actor, NetConnection* connection, NetDriver* netDriver) {
        std::vector<ReplicatedProperty*> properties;
        actor->GetReplicatedProperties(properties);

        // Check if any properties have changed
        bool hasChanges = false;
        for (auto* prop : properties) {
            if (prop->HasChanged()) {
                hasChanges = true;
                break;
            }
        }

        if (!hasChanges) {
            return; // No changes, skip update
        }

        // Create replication packet
        Packet packet(PacketType::ActorReplication);
        packet.Write(actor->GetNetId());

        // Count changed properties
        uint32_t changedCount = 0;
        for (auto* prop : properties) {
            if (prop->HasChanged()) {
                changedCount++;
            }
        }

        packet.Write(changedCount);

        // Serialize changed properties
        for (auto* prop : properties) {
            if (prop->HasChanged()) {
                prop->Serialize(packet.GetPayload());
                prop->UpdateLastValue();
            }
        }

        netDriver->SendPacket(connection, packet, true);
    }

    void ReplicationManager::HandleActorSpawn(NetConnection* connection, const Packet& packet) {
        uint32_t netId = packet.GetPayload().ReadUInt32();
        std::string typeName = packet.GetPayload().ReadString();
        glm::vec3 position = packet.GetPayload().ReadVector3();
        glm::quat rotation = packet.GetPayload().ReadQuaternion();

        // Spawn actor on client
        Actor* actor = World::Get().SpawnActorByType(typeName);
        if (actor) {
            actor->SetNetId(netId);
            actor->SetPosition(position);
            actor->SetRotation(rotation);
            actor->SetReplicates(true);
        }
    }

    void ReplicationManager::HandleActorDestroy(NetConnection* connection, const Packet& packet) {
        uint32_t netId = packet.GetPayload().ReadUInt32();
        World::Get().DestroyActorById(netId);
    }

    void ReplicationManager::HandleActorUpdate(NetConnection* connection, const Packet& packet) {
        uint32_t netId = packet.GetPayload().ReadUInt32();
        uint32_t propertyCount = packet.GetPayload().ReadUInt32();

        Actor* actor = World::Get().GetActorByNetId(netId);
        if (!actor) {
            return;
        }

        // Deserialize properties
        auto& properties = actor->GetRegisteredProperties();
        for (uint32_t i = 0; i < propertyCount; ++i) {
            std::string propName = packet.GetPayload().ReadString();
            PropertyType propType = static_cast<PropertyType>(packet.GetPayload().ReadUInt8());

            auto it = properties.find(propName);
            if (it != properties.end()) {
                // Rewind to read property type again (Deserialize expects it)
                size_t readPos = packet.GetPayload().GetReadPos();
                const_cast<BitStream&>(packet.GetPayload()).ResetReadPos();
                // Skip to the property type we just read
                for (size_t j = 0; j < readPos - 1; ++j) {
                    packet.GetPayload().ReadUInt8();
                }

                it->second.Deserialize(const_cast<BitStream&>(packet.GetPayload()));
            }
        }

        actor->OnReplicated();
    }

    ActorReplicationState* ReplicationManager::GetOrCreateReplicationState(NetConnection* connection, uint32_t actorNetId) {
        auto& actorStates = m_connectionStates[connection];
        auto it = actorStates.find(actorNetId);

        if (it == actorStates.end()) {
            ActorReplicationState state;
            state.actorNetId = actorNetId;
            actorStates[actorNetId] = state;
            return &actorStates[actorNetId];
        }

        return &it->second;
    }

} // namespace WVNet
