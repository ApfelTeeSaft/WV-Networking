#pragma once

#include <wvnet/Core.h>
#include <wvnet/BitStream.h>

namespace WVNet {

    //=============================================================================
    // PacketType - Defines different types of network packets
    //=============================================================================

    enum class PacketType : uint16_t {
        // Connection management
        ConnectionRequest = 0,
        ConnectionAccept = 1,
        ConnectionDenied = 2,
        Disconnect = 3,

        // Reliability
        Acknowledgement = 10,
        Heartbeat = 11,

        // Actor/Object replication
        ActorSpawn = 20,
        ActorDestroy = 21,
        ActorReplication = 22,

        // RPC
        RPCServer = 30,
        RPCClient = 31,
        RPCMulticast = 32,

        // Control
        TimeSync = 100,
    };

    //=============================================================================
    // PacketHeader - Fixed-size header for all packets
    //=============================================================================

    struct PacketHeader {
        uint32_t magic;        // 'WVNE' magic number
        uint32_t sequence;     // Sequence number
        uint16_t packetType;   // Type of packet
        uint16_t payloadSize;  // Size of payload in bytes

        PacketHeader()
            : magic(PACKET_MAGIC), sequence(0), packetType(0), payloadSize(0) {}

        void Serialize(BitStream& stream) const {
            stream.WriteUInt32(magic);
            stream.WriteUInt32(sequence);
            stream.WriteUInt16(packetType);
            stream.WriteUInt16(payloadSize);
        }

        bool Deserialize(BitStream& stream) {
            magic = stream.ReadUInt32();
            sequence = stream.ReadUInt32();
            packetType = stream.ReadUInt16();
            payloadSize = stream.ReadUInt16();
            return magic == PACKET_MAGIC;
        }

        static constexpr size_t GetSize() {
            return sizeof(uint32_t) * 2 + sizeof(uint16_t) * 2; // 12 bytes
        }
    };

    //=============================================================================
    // Packet - Network packet with header and payload
    //=============================================================================

    class Packet {
    public:
        Packet();
        explicit Packet(PacketType type);

        // Header access
        PacketHeader& GetHeader() { return m_header; }
        const PacketHeader& GetHeader() const { return m_header; }

        void SetType(PacketType type);
        PacketType GetType() const;

        void SetSequence(uint32_t sequence);
        uint32_t GetSequence() const;

        // Payload access
        BitStream& GetPayload() { return m_payload; }
        const BitStream& GetPayload() const { return m_payload; }

        // Serialization
        void Serialize(BitStream& outStream) const;
        bool Deserialize(BitStream& inStream);

        // Convenience methods for writing to payload
        template<typename T>
        void Write(const T& value) {
            if constexpr (std::is_same_v<T, bool>) {
                m_payload.WriteBool(value);
            } else if constexpr (std::is_same_v<T, int8_t>) {
                m_payload.WriteInt8(value);
            } else if constexpr (std::is_same_v<T, uint8_t>) {
                m_payload.WriteUInt8(value);
            } else if constexpr (std::is_same_v<T, int16_t>) {
                m_payload.WriteInt16(value);
            } else if constexpr (std::is_same_v<T, uint16_t>) {
                m_payload.WriteUInt16(value);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                m_payload.WriteInt32(value);
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                m_payload.WriteUInt32(value);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                m_payload.WriteInt64(value);
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                m_payload.WriteUInt64(value);
            } else if constexpr (std::is_same_v<T, float>) {
                m_payload.WriteFloat(value);
            } else if constexpr (std::is_same_v<T, double>) {
                m_payload.WriteDouble(value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                m_payload.WriteString(value);
            } else if constexpr (std::is_same_v<T, glm::vec3>) {
                m_payload.WriteVector3(value);
            } else if constexpr (std::is_same_v<T, glm::quat>) {
                m_payload.WriteQuaternion(value);
            }
        }

    private:
        PacketHeader m_header;
        BitStream m_payload;
    };

} // namespace WVNet
