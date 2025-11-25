#include <wvnet/Actor.h>
#include <wvnet/World.h>
#include <wvnet/BitStream.h>
#include <wvnet/NetworkManager.h>
#include <cstring>

namespace WVNet {

    //=============================================================================
    // ReplicatedProperty Implementation
    //=============================================================================

    bool ReplicatedProperty::HasChanged() const {
        if (lastValue.empty() || !dataPtr) {
            return true;
        }
        return memcmp(dataPtr, lastValue.data(), size) != 0;
    }

    void ReplicatedProperty::UpdateLastValue() {
        if (dataPtr && !lastValue.empty()) {
            memcpy(lastValue.data(), dataPtr, size);
            dirty = false;
        }
    }

    void ReplicatedProperty::Serialize(BitStream& stream) const {
        if (!dataPtr) return;

        stream.WriteString(name);
        stream.WriteUInt8(static_cast<uint8_t>(type));

        switch (type) {
            case PropertyType::Bool:
                stream.WriteBool(*static_cast<bool*>(dataPtr));
                break;
            case PropertyType::Int8:
                stream.WriteInt8(*static_cast<int8_t*>(dataPtr));
                break;
            case PropertyType::UInt8:
                stream.WriteUInt8(*static_cast<uint8_t*>(dataPtr));
                break;
            case PropertyType::Int16:
                stream.WriteInt16(*static_cast<int16_t*>(dataPtr));
                break;
            case PropertyType::UInt16:
                stream.WriteUInt16(*static_cast<uint16_t*>(dataPtr));
                break;
            case PropertyType::Int32:
                stream.WriteInt32(*static_cast<int32_t*>(dataPtr));
                break;
            case PropertyType::UInt32:
                stream.WriteUInt32(*static_cast<uint32_t*>(dataPtr));
                break;
            case PropertyType::Int64:
                stream.WriteInt64(*static_cast<int64_t*>(dataPtr));
                break;
            case PropertyType::UInt64:
                stream.WriteUInt64(*static_cast<uint64_t*>(dataPtr));
                break;
            case PropertyType::Float:
                stream.WriteFloat(*static_cast<float*>(dataPtr));
                break;
            case PropertyType::Double:
                stream.WriteDouble(*static_cast<double*>(dataPtr));
                break;
            case PropertyType::Vector3:
                stream.WriteVector3(*static_cast<glm::vec3*>(dataPtr));
                break;
            case PropertyType::Quaternion:
                stream.WriteQuaternion(*static_cast<glm::quat*>(dataPtr));
                break;
            case PropertyType::String:
                stream.WriteString(*static_cast<std::string*>(dataPtr));
                break;
            default:
                break;
        }
    }

    void ReplicatedProperty::Deserialize(BitStream& stream) {
        if (!dataPtr) return;

        name = stream.ReadString();
        type = static_cast<PropertyType>(stream.ReadUInt8());

        switch (type) {
            case PropertyType::Bool:
                *static_cast<bool*>(dataPtr) = stream.ReadBool();
                break;
            case PropertyType::Int8:
                *static_cast<int8_t*>(dataPtr) = stream.ReadInt8();
                break;
            case PropertyType::UInt8:
                *static_cast<uint8_t*>(dataPtr) = stream.ReadUInt8();
                break;
            case PropertyType::Int16:
                *static_cast<int16_t*>(dataPtr) = stream.ReadInt16();
                break;
            case PropertyType::UInt16:
                *static_cast<uint16_t*>(dataPtr) = stream.ReadUInt16();
                break;
            case PropertyType::Int32:
                *static_cast<int32_t*>(dataPtr) = stream.ReadInt32();
                break;
            case PropertyType::UInt32:
                *static_cast<uint32_t*>(dataPtr) = stream.ReadUInt32();
                break;
            case PropertyType::Int64:
                *static_cast<int64_t*>(dataPtr) = stream.ReadInt64();
                break;
            case PropertyType::UInt64:
                *static_cast<uint64_t*>(dataPtr) = stream.ReadUInt64();
                break;
            case PropertyType::Float:
                *static_cast<float*>(dataPtr) = stream.ReadFloat();
                break;
            case PropertyType::Double:
                *static_cast<double*>(dataPtr) = stream.ReadDouble();
                break;
            case PropertyType::Vector3:
                *static_cast<glm::vec3*>(dataPtr) = stream.ReadVector3();
                break;
            case PropertyType::Quaternion:
                *static_cast<glm::quat*>(dataPtr) = stream.ReadQuaternion();
                break;
            case PropertyType::String:
                *static_cast<std::string*>(dataPtr) = stream.ReadString();
                break;
            default:
                break;
        }

        UpdateLastValue();
    }

    //=============================================================================
    // Actor Implementation
    //=============================================================================

    Actor::Actor()
        : m_netId(0)
        , m_replicates(false)
        , m_world(nullptr)
        , m_position(0.0f)
        , m_rotation(1.0f, 0.0f, 0.0f, 0.0f)
        , m_scale(1.0f) {
    }

    Actor::~Actor() {
    }

    void Actor::SetReplicates(bool replicates) {
        m_replicates = replicates;
    }

    bool Actor::IsNetworked() const {
        return m_replicates && m_netId != 0;
    }

    void Actor::SetPosition(const glm::vec3& pos) {
        m_position = pos;
    }

    void Actor::SetRotation(const glm::quat& rot) {
        m_rotation = rot;
    }

    void Actor::SetScale(const glm::vec3& scale) {
        m_scale = scale;
    }

    void Actor::GetReplicatedProperties(std::vector<ReplicatedProperty*>& outProps) {
        for (auto& [name, prop] : m_replicatedProperties) {
            outProps.push_back(&prop);
        }
    }

    void Actor::RegisterReplicatedProperty(const std::string& name, void* ptr, PropertyType type, size_t size) {
        ReplicatedProperty prop(name, type, ptr, size);
        m_replicatedProperties[name] = prop;
    }

} // namespace WVNet
