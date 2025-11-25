#pragma once

#include <wvnet/Core.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace WVNet {

    // Forward declarations
    class World;
    class BitStream;

    //=============================================================================
    // PropertyType - Types of replicated properties
    //=============================================================================

    enum class PropertyType {
        Bool,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Float,
        Double,
        Vector3,
        Quaternion,
        String,
        Custom
    };

    //=============================================================================
    // ReplicatedProperty - Metadata for a replicated property
    //=============================================================================

    struct ReplicatedProperty {
        std::string name;
        PropertyType type;
        void* dataPtr;              // Pointer to actual property in actor
        size_t size;                // Size in bytes
        std::vector<uint8_t> lastValue; // Last replicated value
        bool dirty;                 // Has changed since last replication

        ReplicatedProperty() : dataPtr(nullptr), size(0), dirty(false) {}

        ReplicatedProperty(const std::string& n, PropertyType t, void* ptr, size_t sz)
            : name(n), type(t), dataPtr(ptr), size(sz), dirty(true) {
            lastValue.resize(sz);
        }

        // Check if current value differs from last replicated value
        bool HasChanged() const;

        // Update last value to current value
        void UpdateLastValue();

        // Serialize current value to stream
        void Serialize(BitStream& stream) const;

        // Deserialize value from stream and update property
        void Deserialize(BitStream& stream);
    };

    //=============================================================================
    // Actor - Base class for networked game objects
    //=============================================================================

    class Actor {
    public:
        Actor();
        virtual ~Actor();

        // Lifecycle
        virtual void OnSpawn() {}
        virtual void OnDestroy() {}
        virtual void Tick(float deltaTime) {}

        // Networking
        void SetReplicates(bool replicates);
        bool GetReplicates() const { return m_replicates; }

        uint32_t GetNetId() const { return m_netId; }
        void SetNetId(uint32_t netId) { m_netId = netId; }

        bool IsNetworked() const;

        // Transform
        void SetPosition(const glm::vec3& pos);
        const glm::vec3& GetPosition() const { return m_position; }

        void SetRotation(const glm::quat& rot);
        const glm::quat& GetRotation() const { return m_rotation; }

        void SetScale(const glm::vec3& scale);
        const glm::vec3& GetScale() const { return m_scale; }

        // World reference
        World* GetWorld() const { return m_world; }
        void SetWorld(World* world) { m_world = world; }

        // Replication
        virtual void GetReplicatedProperties(std::vector<ReplicatedProperty*>& outProps);
        virtual void OnReplicated() {}

        // Property registration (to be called in derived class constructors)
        void RegisterReplicatedProperty(const std::string& name, void* ptr, PropertyType type, size_t size);

        // Get registered properties
        const std::unordered_map<std::string, ReplicatedProperty>& GetRegisteredProperties() const {
            return m_replicatedProperties;
        }

        std::unordered_map<std::string, ReplicatedProperty>& GetRegisteredProperties() {
            return m_replicatedProperties;
        }

        // Actor type name (for serialization/spawning)
        virtual std::string GetTypeName() const { return "Actor"; }

    protected:
        // Helper templates for property registration
        template<typename T>
        void RegisterProperty(const std::string& name, T* ptr) {
            PropertyType type = GetPropertyType<T>();
            RegisterReplicatedProperty(name, ptr, type, sizeof(T));
        }

        template<typename T>
        static PropertyType GetPropertyType() {
            if constexpr (std::is_same_v<T, bool>) return PropertyType::Bool;
            else if constexpr (std::is_same_v<T, int8_t>) return PropertyType::Int8;
            else if constexpr (std::is_same_v<T, uint8_t>) return PropertyType::UInt8;
            else if constexpr (std::is_same_v<T, int16_t>) return PropertyType::Int16;
            else if constexpr (std::is_same_v<T, uint16_t>) return PropertyType::UInt16;
            else if constexpr (std::is_same_v<T, int32_t>) return PropertyType::Int32;
            else if constexpr (std::is_same_v<T, uint32_t>) return PropertyType::UInt32;
            else if constexpr (std::is_same_v<T, int64_t>) return PropertyType::Int64;
            else if constexpr (std::is_same_v<T, uint64_t>) return PropertyType::UInt64;
            else if constexpr (std::is_same_v<T, float>) return PropertyType::Float;
            else if constexpr (std::is_same_v<T, double>) return PropertyType::Double;
            else if constexpr (std::is_same_v<T, glm::vec3>) return PropertyType::Vector3;
            else if constexpr (std::is_same_v<T, glm::quat>) return PropertyType::Quaternion;
            else if constexpr (std::is_same_v<T, std::string>) return PropertyType::String;
            else return PropertyType::Custom;
        }

    private:
        uint32_t m_netId;
        bool m_replicates;
        World* m_world;

        // Transform
        glm::vec3 m_position;
        glm::quat m_rotation;
        glm::vec3 m_scale;

        // Replicated properties
        std::unordered_map<std::string, ReplicatedProperty> m_replicatedProperties;
    };

} // namespace WVNet
