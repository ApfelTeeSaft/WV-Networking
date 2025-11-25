#pragma once

#include <wvnet/Core.h>
#include <wvnet/platform/Socket.h>
#include <wvnet/Packet.h>
#include <queue>
#include <map>

namespace WVNet {

    //=============================================================================
    // ConnectionState - Represents the state of a network connection
    //=============================================================================

    enum class ConnectionState {
        Connecting,      // Connection in progress
        Connected,       // Connection established
        Disconnecting,   // Disconnection in progress
        Disconnected     // Connection closed
    };

    //=============================================================================
    // NetConnection - Represents a single network connection
    //=============================================================================

    class NetConnection {
    public:
        explicit NetConnection(const WVSocketAddress& address);
        ~NetConnection();

        // Sending
        void SendPacket(const Packet& packet, bool reliable = true);
        void FlushOutgoing(WVSocket* socket);

        // Receiving
        void ReceivePacket(const Packet& packet);

        // Update
        void Tick(float deltaTime);

        // State
        ConnectionState GetState() const { return m_state; }
        void SetState(ConnectionState state) { m_state = state; }

        const WVSocketAddress& GetAddress() const { return m_address; }
        float GetRoundTripTime() const { return m_roundTripTime; }
        float GetTimeSinceLastReceive() const;

        // Sequence management
        uint32_t GetNextOutgoingSequence();
        uint32_t GetIncomingSequence() const { return m_incomingSequence; }

        // Timeout detection
        bool IsTimedOut(float timeout) const;

        // User data (e.g., player actor reference)
        void SetUserData(void* data) { m_userData = data; }
        void* GetUserData() const { return m_userData; }

        // Statistics
        struct Stats {
            uint64_t packetsSent = 0;
            uint64_t packetsReceived = 0;
            uint64_t bytesSent = 0;
            uint64_t bytesReceived = 0;
            uint32_t packetsLost = 0;
        };
        const Stats& GetStats() const { return m_stats; }

    private:
        void ProcessAcknowledgement(const Packet& packet);
        void SendAcknowledgement(uint32_t sequence);

        WVSocketAddress m_address;
        ConnectionState m_state;

        // Sequencing
        uint32_t m_outgoingSequence;
        uint32_t m_incomingSequence;

        // Reliable packet handling
        std::map<uint32_t, Packet> m_reliableBuffer;  // Packets awaiting ack
        std::queue<Packet> m_outgoingQueue;

        // Timing
        float m_roundTripTime;
        float m_lastSendTime;
        float m_lastReceiveTime;
        float m_currentTime;

        // User data
        void* m_userData;

        // Statistics
        Stats m_stats;
    };

} // namespace WVNet
