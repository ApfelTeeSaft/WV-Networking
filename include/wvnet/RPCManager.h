#pragma once

#include <wvnet/Core.h>
#include <wvnet/Packet.h>
#include <wvnet/NetConnection.h>
#include <wvnet/Actor.h>
#include <wvnet/BitStream.h>

namespace WVNet {

    //=============================================================================
    // RPCType - Type of RPC call
    //=============================================================================

    enum class RPCType : uint8_t {
        Server,      // Called on client, executed on server
        Client,      // Called on server, executed on specific client
        Multicast    // Called on server, executed on all clients
    };

    //=============================================================================
    // RPCHandler - Function signature for RPC handlers
    //=============================================================================

    using RPCHandler = std::function<void(Actor*, BitStream&)>;

    //=============================================================================
    // RPCMetadata - Metadata for a registered RPC
    //=============================================================================

    struct RPCMetadata {
        std::string name;
        RPCType type;
        RPCHandler handler;

        RPCMetadata() : type(RPCType::Server) {}
        RPCMetadata(const std::string& n, RPCType t, RPCHandler h)
            : name(n), type(t), handler(h) {}
    };

    //=============================================================================
    // RPCManager - Manages RPC registration and dispatch
    //=============================================================================

    class RPCManager {
    public:
        RPCManager();
        ~RPCManager();

        // Registration
        void RegisterRPC(const std::string& functionName, RPCType type, RPCHandler handler);

        // Invocation
        void CallServerRPC(Actor* actor, const std::string& functionName, BitStream& params);
        void CallClientRPC(Actor* actor, NetConnection* client, const std::string& functionName, BitStream& params);
        void CallMulticastRPC(Actor* actor, const std::string& functionName, BitStream& params);

        // Processing
        void ProcessRPC(NetConnection* connection, const Packet& packet, class NetDriver* netDriver);

    private:
        void SendRPC(NetConnection* connection, PacketType packetType, uint32_t actorNetId,
                    const std::string& functionName, BitStream& params, class NetDriver* netDriver);

        std::unordered_map<std::string, RPCMetadata> m_rpcRegistry;
    };

    //=============================================================================
    // RPC Macros for convenience
    //=============================================================================

    // These macros can be used in actor classes to mark RPC functions
    // Implementation is simplified - in a proper system, you'd use more
    // sophisticated macro/template metaprogramming

    #define WV_RPC_SERVER    // Marker for Server RPC (documentation)
    #define WV_RPC_CLIENT    // Marker for Client RPC (documentation)
    #define WV_RPC_MULTICAST // Marker for Multicast RPC (documentation)

} // namespace WVNet
