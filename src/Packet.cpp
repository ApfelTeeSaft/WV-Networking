#include <wvnet/Packet.h>

namespace WVNet {

    Packet::Packet() {
        m_header.magic = PACKET_MAGIC;
        m_header.sequence = 0;
        m_header.packetType = 0;
        m_header.payloadSize = 0;
    }

    Packet::Packet(PacketType type) : Packet() {
        SetType(type);
    }

    void Packet::SetType(PacketType type) {
        m_header.packetType = static_cast<uint16_t>(type);
    }

    PacketType Packet::GetType() const {
        return static_cast<PacketType>(m_header.packetType);
    }

    void Packet::SetSequence(uint32_t sequence) {
        m_header.sequence = sequence;
    }

    uint32_t Packet::GetSequence() const {
        return m_header.sequence;
    }

    void Packet::Serialize(BitStream& outStream) const {
        // Update payload size in header
        const_cast<PacketHeader&>(m_header).payloadSize = static_cast<uint16_t>(m_payload.GetSize());

        // Serialize header
        m_header.Serialize(outStream);

        // Serialize payload
        if (m_payload.GetSize() > 0) {
            outStream.Write(m_payload.GetData(), m_payload.GetSize());
        }
    }

    bool Packet::Deserialize(BitStream& inStream) {
        // Deserialize header
        if (!m_header.Deserialize(inStream)) {
            WVNET_LOG_ERROR("Invalid packet magic number");
            return false;
        }

        // Deserialize payload
        if (m_header.payloadSize > 0) {
            if (!inStream.CanRead(m_header.payloadSize)) {
                WVNET_LOG_ERROR("Packet payload size mismatch");
                return false;
            }

            std::vector<uint8_t> payloadData(m_header.payloadSize);
            inStream.Read(payloadData.data(), m_header.payloadSize);

            m_payload = BitStream(payloadData.data(), m_header.payloadSize);
        }

        return true;
    }

} // namespace WVNet
