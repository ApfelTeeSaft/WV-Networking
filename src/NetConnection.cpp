#include <wvnet/NetConnection.h>

namespace WVNet {

    NetConnection::NetConnection(const WVSocketAddress& address)
        : m_address(address)
        , m_state(ConnectionState::Connecting)
        , m_outgoingSequence(0)
        , m_incomingSequence(0)
        , m_roundTripTime(0.0f)
        , m_lastSendTime(0.0f)
        , m_lastReceiveTime(0.0f)
        , m_currentTime(0.0f)
        , m_userData(nullptr) {
    }

    NetConnection::~NetConnection() {
    }

    void NetConnection::SendPacket(const Packet& packet, bool reliable) {
        // Assign sequence number
        Packet outPacket = packet;
        outPacket.SetSequence(GetNextOutgoingSequence());

        // Add to outgoing queue
        m_outgoingQueue.push(outPacket);

        // If reliable, also store in buffer for potential retransmission
        if (reliable) {
            m_reliableBuffer[outPacket.GetSequence()] = outPacket;
        }
    }

    void NetConnection::FlushOutgoing(WVSocket* socket) {
        if (!socket || !socket->IsValid()) {
            return;
        }

        while (!m_outgoingQueue.empty()) {
            Packet& packet = m_outgoingQueue.front();

            // Serialize packet
            BitStream stream;
            packet.Serialize(stream);

            // Send
            int32_t bytesSent = socket->SendTo(stream.GetData(), stream.GetSize(), m_address);

            if (bytesSent > 0) {
                m_stats.packetsSent++;
                m_stats.bytesSent += bytesSent;
                m_lastSendTime = m_currentTime;
                m_outgoingQueue.pop();
            } else {
                // Would block or error, try again later
                break;
            }
        }
    }

    void NetConnection::ReceivePacket(const Packet& packet) {
        m_lastReceiveTime = m_currentTime;
        m_stats.packetsReceived++;

        // Update incoming sequence
        uint32_t sequence = packet.GetSequence();
        if (sequence > m_incomingSequence) {
            m_incomingSequence = sequence;
        }

        // Send acknowledgement for reliable packets
        PacketType type = packet.GetType();
        if (type != PacketType::Acknowledgement && type != PacketType::Heartbeat) {
            SendAcknowledgement(sequence);
        }

        // Handle acknowledgement packets
        if (type == PacketType::Acknowledgement) {
            ProcessAcknowledgement(packet);
        }

        // TODO: Handle other packet types (will be done by NetDriver/managers)
    }

    void NetConnection::Tick(float deltaTime) {
        m_currentTime += deltaTime;

        // TODO: Retransmit reliable packets if not acknowledged within timeout
        // TODO: Send heartbeat if no data sent recently
    }

    float NetConnection::GetTimeSinceLastReceive() const {
        return m_currentTime - m_lastReceiveTime;
    }

    uint32_t NetConnection::GetNextOutgoingSequence() {
        return m_outgoingSequence++;
    }

    bool NetConnection::IsTimedOut(float timeout) const {
        return GetTimeSinceLastReceive() > timeout;
    }

    void NetConnection::ProcessAcknowledgement(const Packet& packet) {
        // Read acknowledged sequence number
        uint32_t ackedSequence = packet.GetPayload().ReadUInt32();

        // Remove from reliable buffer
        auto it = m_reliableBuffer.find(ackedSequence);
        if (it != m_reliableBuffer.end()) {
            m_reliableBuffer.erase(it);

            // Update RTT (simplified)
            float rtt = m_currentTime - m_lastSendTime;
            m_roundTripTime = m_roundTripTime * 0.9f + rtt * 0.1f; // Exponential moving average
        }
    }

    void NetConnection::SendAcknowledgement(uint32_t sequence) {
        Packet ackPacket(PacketType::Acknowledgement);
        ackPacket.Write(sequence);
        SendPacket(ackPacket, false); // Acks themselves don't need to be reliable
    }

} // namespace WVNet
