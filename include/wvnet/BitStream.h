#pragma once

#include <wvnet/Core.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace WVNet {

    //=============================================================================
    // BitStream - Helper for serializing/deserializing data
    //=============================================================================

    class BitStream {
    public:
        BitStream();
        explicit BitStream(size_t reserveSize);
        BitStream(const uint8_t* data, size_t size);

        // Writing
        void Write(const void* data, size_t size);
        void WriteBool(bool value);
        void WriteInt8(int8_t value);
        void WriteUInt8(uint8_t value);
        void WriteInt16(int16_t value);
        void WriteUInt16(uint16_t value);
        void WriteInt32(int32_t value);
        void WriteUInt32(uint32_t value);
        void WriteInt64(int64_t value);
        void WriteUInt64(uint64_t value);
        void WriteFloat(float value);
        void WriteDouble(double value);
        void WriteString(const std::string& value);
        void WriteVector3(const glm::vec3& value);
        void WriteQuaternion(const glm::quat& value);

        // Reading
        bool Read(void* data, size_t size);
        bool ReadBool();
        int8_t ReadInt8();
        uint8_t ReadUInt8();
        int16_t ReadInt16();
        uint16_t ReadUInt16();
        int32_t ReadInt32();
        uint32_t ReadUInt32();
        int64_t ReadInt64();
        uint64_t ReadUInt64();
        float ReadFloat();
        double ReadDouble();
        std::string ReadString();
        glm::vec3 ReadVector3();
        glm::quat ReadQuaternion();

        // State
        const uint8_t* GetData() const { return m_buffer.data(); }
        size_t GetSize() const { return m_writePos; }
        size_t GetReadPos() const { return m_readPos; }
        size_t GetBytesRemaining() const { return m_writePos - m_readPos; }
        bool CanRead(size_t bytes) const { return m_readPos + bytes <= m_writePos; }

        // Reset
        void Clear();
        void ResetReadPos();

    private:
        void EnsureCapacity(size_t additionalBytes);

        std::vector<uint8_t> m_buffer;
        size_t m_writePos;
        size_t m_readPos;
    };

} // namespace WVNet
