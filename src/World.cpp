#include <wvnet/World.h>
#include <algorithm>

namespace WVNet {

    World& World::Get() {
        static World instance;
        return instance;
    }

    World::World() : m_nextNetId(1) {
    }

    World::~World() {
        Clear();
    }

    void World::Tick(float deltaTime) {
        // Tick all actors
        for (auto* actor : m_actorList) {
            actor->Tick(deltaTime);
        }

        // Process pending destroys
        for (auto* actor : m_pendingDestroy) {
            actor->OnDestroy();

            // Remove from lookup maps
            m_actorsByNetId.erase(actor->GetNetId());

            // Remove from list
            m_actorList.erase(
                std::remove(m_actorList.begin(), m_actorList.end(), actor),
                m_actorList.end()
            );

            // Remove from owned actors
            m_actors.erase(
                std::remove_if(m_actors.begin(), m_actors.end(),
                    [actor](const std::unique_ptr<Actor>& a) {
                        return a.get() == actor;
                    }),
                m_actors.end()
            );
        }

        m_pendingDestroy.clear();
    }

    Actor* World::SpawnActor(std::unique_ptr<Actor> actor) {
        if (!actor) {
            return nullptr;
        }

        // Assign network ID
        actor->SetNetId(GenerateNetId());
        actor->SetWorld(this);

        Actor* rawPtr = actor.get();

        // Add to containers
        m_actorsByNetId[rawPtr->GetNetId()] = rawPtr;
        m_actorList.push_back(rawPtr);
        m_actors.push_back(std::move(actor));

        // Call spawn callback
        rawPtr->OnSpawn();

        return rawPtr;
    }

    void World::DestroyActor(Actor* actor) {
        if (!actor) {
            return;
        }

        // Add to pending destroy list (will be destroyed at end of tick)
        if (std::find(m_pendingDestroy.begin(), m_pendingDestroy.end(), actor) == m_pendingDestroy.end()) {
            m_pendingDestroy.push_back(actor);
        }
    }

    void World::DestroyActorById(uint32_t netId) {
        Actor* actor = GetActorByNetId(netId);
        if (actor) {
            DestroyActor(actor);
        }
    }

    Actor* World::GetActorByNetId(uint32_t netId) const {
        auto it = m_actorsByNetId.find(netId);
        if (it != m_actorsByNetId.end()) {
            return it->second;
        }
        return nullptr;
    }

    void World::RegisterActorType(const std::string& typeName, ActorFactory factory) {
        m_actorFactories[typeName] = factory;
        WVNET_LOG_FMT("Registered actor type: %s", typeName.c_str());
    }

    Actor* World::SpawnActorByType(const std::string& typeName) {
        auto it = m_actorFactories.find(typeName);
        if (it == m_actorFactories.end()) {
            WVNET_LOG_FMT("Failed to spawn actor: type '%s' not registered", typeName.c_str());
            return nullptr;
        }

        auto actor = it->second();
        return SpawnActor(std::move(actor));
    }

    void World::Clear() {
        // Destroy all actors
        for (auto& actor : m_actors) {
            actor->OnDestroy();
        }

        m_actors.clear();
        m_actorList.clear();
        m_actorsByNetId.clear();
        m_pendingDestroy.clear();
        m_nextNetId = 1;
    }

    uint32_t World::GenerateNetId() {
        return m_nextNetId++;
    }

} // namespace WVNet
