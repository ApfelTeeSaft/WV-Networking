#pragma once

#include <wvnet/Core.h>
#include <wvnet/Actor.h>
#include <wvnet/NetConnection.h>
#include <wvnet/Packet.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace WVNet {

    //=============================================================================
    // ActorReplicationState - Per-actor, per-connection replication state
    //=============================================================================

    struct ActorReplicationState {
        uint32_t actorNetId;
        bool spawned;  // Has this actor been spawned on the client?
        float lastReplicationTime;
        std::unordered_map<std::string, std::vector<uint8_t>> lastPropertyValues;

        ActorReplicationState() : actorNetId(0), spawned(false), lastReplicationTime(0.0f) {}
    };

    //=============================================================================
    // ReplicationManager - Manages actor replication to clients
    //=============================================================================

    class ReplicationManager {
    public:
        ReplicationManager();
        ~ReplicationManager();

        void Initialize(float tickRate);
        void Tick(float deltaTime, class NetDriver* netDriver);

        // Actor registration
        void RegisterActor(Actor* actor);
        void UnregisterActor(Actor* actor);

        // Replication
        void ReplicateActors(NetConnection* connection, class NetDriver* netDriver);
        void ProcessActorReplication(NetConnection* connection, const Packet& packet);

        // Relevancy
        bool IsActorRelevantForConnection(Actor* actor, NetConnection* connection);
        void SetRelevancyDistance(float distance);

        // Configuration
        void SetTickRate(float tickRate);
        float GetTickRate() const { return m_tickRate; }

    private:
        void SendActorSpawn(Actor* actor, NetConnection* connection, class NetDriver* netDriver);
        void SendActorDestroy(uint32_t actorNetId, NetConnection* connection, class NetDriver* netDriver);
        void SendActorUpdate(Actor* actor, NetConnection* connection, class NetDriver* netDriver);

        void HandleActorSpawn(NetConnection* connection, const Packet& packet);
        void HandleActorDestroy(NetConnection* connection, const Packet& packet);
        void HandleActorUpdate(NetConnection* connection, const Packet& packet);

        ActorReplicationState* GetOrCreateReplicationState(NetConnection* connection, uint32_t actorNetId);

        std::vector<Actor*> m_replicatedActors;
        float m_tickRate;
        float m_replicationInterval;
        float m_timeSinceLastReplication;
        float m_relevancyDistance;

        // Per-connection replication state
        std::unordered_map<NetConnection*, std::unordered_map<uint32_t, ActorReplicationState>> m_connectionStates;
    };

} // namespace WVNet
