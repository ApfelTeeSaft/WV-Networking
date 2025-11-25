#include <wvnet/BitStream.h>
#include <cstring>

namespace WVNet {

    BitStream::BitStream() : m_writePos(0), m_readPos(0) {
        m_buffer.reserve(256); // Default capacity
    }

    BitStream::BitStream(size_t reserveSize) : m_writePos(0), m_readPos(0) {
        m_buffer.reserve(reserveSize);
    }

    BitStream::BitStream(const uint8_t* data, size_t size)
        : m_buffer(data, data + size), m_writePos(size), m_readPos(0) {
    }

    void BitStream::Write(const void* data, size_t size) {
        EnsureCapacity(size);
        memcpy(m_buffer.data() + m_writePos, data, size);
        m_writePos += size;
    }

    void BitStream::WriteBool(bool value) {
        WriteUInt8(value ? 1 : 0);
    }

    void BitStream::WriteInt8(int8_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteUInt8(uint8_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteInt16(int16_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteUInt16(uint16_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteInt32(int32_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteUInt32(uint32_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteInt64(int64_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteUInt64(uint64_t value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteFloat(float value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteDouble(double value) {
        Write(&value, sizeof(value));
    }

    void BitStream::WriteString(const std::string& value) {
        uint32_t length = static_cast<uint32_t>(value.length());
        WriteUInt32(length);
        if (length > 0) {
            Write(value.data(), length);
        }
    }

    void BitStream::WriteVector3(const glm::vec3& value) {
        WriteFloat(value.x);
        WriteFloat(value.y);
        WriteFloat(value.z);
    }

    void BitStream::WriteQuaternion(const glm::quat& value) {
        WriteFloat(value.w);
        WriteFloat(value.x);
        WriteFloat(value.y);
        WriteFloat(value.z);
    }

    bool BitStream::Read(void* data, size_t size) {
        if (!CanRead(size)) {
            return false;
        }
        memcpy(data, m_buffer.data() + m_readPos, size);
        m_readPos += size;
        return true;
    }

    bool BitStream::ReadBool() {
        return ReadUInt8() != 0;
    }

    int8_t BitStream::ReadInt8() {
        int8_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    uint8_t BitStream::ReadUInt8() {
        uint8_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    int16_t BitStream::ReadInt16() {
        int16_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    uint16_t BitStream::ReadUInt16() {
        uint16_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    int32_t BitStream::ReadInt32() {
        int32_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    uint32_t BitStream::ReadUInt32() {
        uint32_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    int64_t BitStream::ReadInt64() {
        int64_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    uint64_t BitStream::ReadUInt64() {
        uint64_t value = 0;
        Read(&value, sizeof(value));
        return value;
    }

    float BitStream::ReadFloat() {
        float value = 0.0f;
        Read(&value, sizeof(value));
        return value;
    }

    double BitStream::ReadDouble() {
        double value = 0.0;
        Read(&value, sizeof(value));
        return value;
    }

    std::string BitStream::ReadString() {
        uint32_t length = ReadUInt32();
        if (length == 0) {
            return std::string();
        }

        if (!CanRead(length)) {
            return std::string();
        }

        std::string result(length, '\0');
        Read(&result[0], length);
        return result;
    }

    glm::vec3 BitStream::ReadVector3() {
        glm::vec3 result;
        result.x = ReadFloat();
        result.y = ReadFloat();
        result.z = ReadFloat();
        return result;
    }

    glm::quat BitStream::ReadQuaternion() {
        glm::quat result;
        result.w = ReadFloat();
        result.x = ReadFloat();
        result.y = ReadFloat();
        result.z = ReadFloat();
        return result;
    }

    void BitStream::Clear() {
        m_writePos = 0;
        m_readPos = 0;
    }

    void BitStream::ResetReadPos() {
        m_readPos = 0;
    }

    void BitStream::EnsureCapacity(size_t additionalBytes) {
        size_t requiredSize = m_writePos + additionalBytes;
        if (requiredSize > m_buffer.size()) {
            m_buffer.resize(requiredSize);
        }
    }

} // namespace WVNet
