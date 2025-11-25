#pragma once

#include <wvnet/Core.h>
#include <wvnet/Actor.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace WVNet {

    //=============================================================================
    // ActorFactory - Factory function for creating actors by type name
    //=============================================================================

    using ActorFactory = std::function<std::unique_ptr<Actor>()>;

    //=============================================================================
    // World - Manages all actors in the game world
    //=============================================================================

    class World {
    public:
        static World& Get();

        World();
        ~World();

        // Tick all actors
        void Tick(float deltaTime);

        // Actor management
        Actor* SpawnActor(std::unique_ptr<Actor> actor);

        template<typename T, typename... Args>
        T* SpawnActor(Args&&... args) {
            auto actor = std::make_unique<T>(std::forward<Args>(args)...);
            T* rawPtr = actor.get();
            SpawnActor(std::move(actor));
            return rawPtr;
        }

        void DestroyActor(Actor* actor);
        void DestroyActorById(uint32_t netId);

        // Actor lookup
        Actor* GetActorByNetId(uint32_t netId) const;
        const std::vector<Actor*>& GetActors() const { return m_actorList; }

        // Actor type registration (for network spawning)
        void RegisterActorType(const std::string& typeName, ActorFactory factory);

        template<typename T>
        void RegisterActorType(const std::string& typeName) {
            RegisterActorType(typeName, []() { return std::make_unique<T>(); });
        }

        Actor* SpawnActorByType(const std::string& typeName);

        // Clear all actors
        void Clear();

    private:
        uint32_t GenerateNetId();

        std::vector<std::unique_ptr<Actor>> m_actors;
        std::vector<Actor*> m_actorList; // Raw pointers for quick iteration
        std::unordered_map<uint32_t, Actor*> m_actorsByNetId;
        std::unordered_map<std::string, ActorFactory> m_actorFactories;

        uint32_t m_nextNetId;
        std::vector<Actor*> m_pendingDestroy; // Actors to destroy at end of tick
    };

} // namespace WVNet
