# WVNet - Networking Architecture Design

## Overview

WVNet is a networking library for WV-Core that implements Unreal Engine-style server-authoritative networking and replication. This document outlines the architecture, design decisions, and implementation strategy.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        WV-Core App                          │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              App::Run() Game Loop                    │   │
│  │  Start() -> Update() -> NetworkTick() -> Render()   │   │
│  └─────────────────────┬────────────────────────────────┘   │
└────────────────────────┼──────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                    WVNet Library                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                 NetworkManager                        │   │
│  │  • Initialize/Shutdown                                │   │
│  │  • Tick (process packets, replicate actors)           │   │
│  │  • Owns NetDriver                                     │   │
│  └────────┬─────────────────────────────────────────────┘   │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                   NetDriver                           │   │
│  │  • Socket management                                  │   │
│  │  • Accept/connect clients                             │   │
│  │  • Send/receive packets                               │   │
│  │  • Owns multiple NetConnections                       │   │
│  └────────┬─────────────────────────────────────────────┘   │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              NetConnection (per client)               │   │
│  │  • Packet buffering & reliability                     │   │
│  │  • Sequence numbers                                   │   │
│  │  • Connection state (connecting/open/closed)          │   │
│  │  • Per-connection replication state                   │   │
│  └────────┬─────────────────────────────────────────────┘   │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │             ReplicationManager                        │   │
│  │  • Track replicated actors                            │   │
│  │  • Compute property deltas                            │   │
│  │  • Relevancy/interest management                      │   │
│  │  • Serialize/deserialize actor state                  │   │
│  └────────┬─────────────────────────────────────────────┘   │
│           │                                                  │
│           ▼                                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                  RPCManager                           │   │
│  │  • Register RPC functions                             │   │
│  │  • Serialize/deserialize RPC calls                    │   │
│  │  • Dispatch RPCs (Server/Client/Multicast)            │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │          Platform Socket Abstraction                  │   │
│  │  • WVSocket, WVSocketAddress                          │   │
│  │  • Windows: WinSock2                                  │   │
│  │  • macOS/Linux: BSD sockets                           │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. NetworkManager (Singleton)

**Responsibilities:**
- Central networking coordinator
- Initialize/shutdown networking subsystem
- Main tick function called from game loop
- Configuration management
- Access point for networking functionality

**API:**
```cpp
class NetworkManager {
public:
    static NetworkManager& Get();

    void Initialize(const NetworkConfig& config);
    void Shutdown();
    void Tick(float deltaTime);

    bool IsServer() const;
    bool IsClient() const;
    bool IsNetworked() const;

    NetDriver* GetNetDriver();
    ReplicationManager* GetReplicationManager();
    RPCManager* GetRPCManager();
};
```

**Configuration:**
```cpp
struct NetworkConfig {
    NetworkMode mode;              // Server, Client, or Standalone
    std::string serverAddress;     // For clients
    uint16_t serverPort;           // Default: 7777
    uint32_t maxConnections;       // For servers, default: 64
    float tickRate;                // Network update rate (Hz), default: 30
    bool enableRelevancy;          // Enable actor relevancy checks
    float relevancyDistance;       // Default relevancy distance
};
```

---

### 2. NetDriver

**Responsibilities:**
- Low-level socket management
- Accept incoming connections (server)
- Connect to server (client)
- Send/receive raw packets
- Manage NetConnection objects
- Packet routing

**API:**
```cpp
class NetDriver {
public:
    bool InitAsServer(uint16_t port, uint32_t maxConnections);
    bool InitAsClient(const std::string& address, uint16_t port);
    void Shutdown();

    void Tick(float deltaTime);
    void SendPacket(NetConnection* connection, const Packet& packet);

    const std::vector<NetConnection*>& GetConnections() const;
    NetConnection* GetServerConnection(); // For clients

private:
    WVSocket* m_socket;
    std::vector<std::unique_ptr<NetConnection>> m_connections;
    NetConnection* m_serverConnection; // Client only
    NetworkMode m_mode;
};
```

---

### 3. NetConnection

**Responsibilities:**
- Represents a single network connection
- Packet reliability and ordering
- Sequence number management
- Connection state tracking
- Per-connection replication state

**API:**
```cpp
enum class ConnectionState {
    Connecting,
    Connected,
    Disconnecting,
    Disconnected
};

class NetConnection {
public:
    NetConnection(const WVSocketAddress& address);

    void SendPacket(const Packet& packet, bool reliable = true);
    void ReceivePacket(const Packet& packet);
    void Tick(float deltaTime);

    ConnectionState GetState() const;
    const WVSocketAddress& GetAddress() const;
    float GetRoundTripTime() const;

    // Replication state per actor per connection
    ActorReplicationState* GetActorReplicationState(uint32_t actorNetId);

private:
    WVSocketAddress m_address;
    ConnectionState m_state;

    // Reliability
    uint32_t m_outgoingSequence;
    uint32_t m_incomingSequence;
    std::map<uint32_t, Packet> m_reliableBuffer; // Awaiting ack
    std::queue<Packet> m_outgoingQueue;

    // Timing
    float m_roundTripTime;
    float m_lastReceiveTime;

    // Replication state
    std::unordered_map<uint32_t, std::unique_ptr<ActorReplicationState>> m_replicationState;
};
```

---

### 4. ReplicationManager

**Responsibilities:**
- Track all replicated actors in the world
- Compute property deltas (what changed since last replication)
- Manage actor relevancy (which clients see which actors)
- Serialize/deserialize actor state
- Handle actor spawn/destroy replication

**API:**
```cpp
class ReplicationManager {
public:
    void Tick(float deltaTime);

    // Actor registration
    void RegisterActor(Actor* actor);
    void UnregisterActor(Actor* actor);

    // Replication
    void ReplicateActors(NetConnection* connection);
    void ProcessActorReplication(const Packet& packet);

    // Relevancy
    bool IsActorRelevantForConnection(Actor* actor, NetConnection* connection);
    void SetRelevancyDistance(float distance);

private:
    std::vector<Actor*> m_replicatedActors;
    float m_replicationInterval; // 1.0 / tickRate
    float m_timeSinceLastReplication;
    float m_relevancyDistance;
};
```

**Actor Replication State (per actor, per connection):**
```cpp
struct ActorReplicationState {
    uint32_t actorNetId;
    std::unordered_map<std::string, ReplicatedProperty> properties;
    float lastReplicationTime;
    bool spawned; // Has this actor been spawned on the client?
};

struct ReplicatedProperty {
    std::string name;
    PropertyType type;
    std::vector<uint8_t> lastValue; // Last replicated value
    bool dirty; // Has it changed?
};
```

---

### 5. RPCManager

**Responsibilities:**
- Register RPC functions with metadata
- Serialize RPC calls with parameters
- Deserialize and dispatch RPC calls
- Handle different RPC types (Server, Client, Multicast)

**API:**
```cpp
enum class RPCType {
    Server,      // Called on client, executed on server
    Client,      // Called on server, executed on specific client
    Multicast    // Called on server, executed on all clients
};

class RPCManager {
public:
    // Registration
    void RegisterRPC(const std::string& functionName, RPCType type,
                     std::function<void(Actor*, BitStream&)> handler);

    // Invocation
    void CallServerRPC(Actor* actor, const std::string& functionName, BitStream& params);
    void CallClientRPC(Actor* actor, NetConnection* client, const std::string& functionName, BitStream& params);
    void CallMulticastRPC(Actor* actor, const std::string& functionName, BitStream& params);

    // Processing
    void ProcessRPC(const Packet& packet, NetConnection* sender);

private:
    struct RPCMetadata {
        std::string name;
        RPCType type;
        std::function<void(Actor*, BitStream&)> handler;
    };

    std::unordered_map<std::string, RPCMetadata> m_rpcRegistry;
};
```

---

### 6. Actor System (New for WV-Core)

Since WV-Core lacks an entity/actor system, we'll introduce a lightweight one:

**Actor Base Class:**
```cpp
class Actor {
public:
    Actor();
    virtual ~Actor();

    virtual void Tick(float deltaTime) {}
    virtual void OnSpawn() {}
    virtual void OnDestroy() {}

    // Networking
    void SetReplicates(bool replicates);
    bool GetReplicates() const;
    uint32_t GetNetId() const;
    bool IsNetworked() const;

    // Transform (basic, can be extended)
    void SetPosition(const glm::vec3& pos);
    glm::vec3 GetPosition() const;
    void SetRotation(const glm::quat& rot);
    glm::quat GetRotation() const;

    // Replication hooks
    virtual void GetReplicatedProperties(std::vector<ReplicatedProperty*>& outProps);
    virtual void OnReplicated() {}

protected:
    // Helpers for derived classes
    void RegisterReplicatedProperty(const std::string& name, void* ptr, PropertyType type);

private:
    uint32_t m_netId;
    bool m_replicates;
    glm::vec3 m_position;
    glm::quat m_rotation;

    std::unordered_map<std::string, ReplicatedProperty> m_replicatedProperties;
};
```

**World/Scene Manager:**
```cpp
class World {
public:
    static World& Get();

    void Tick(float deltaTime);

    Actor* SpawnActor(std::unique_ptr<Actor> actor);
    void DestroyActor(Actor* actor);
    Actor* GetActorByNetId(uint32_t netId);

    const std::vector<Actor*>& GetActors() const;

private:
    std::vector<std::unique_ptr<Actor>> m_actors;
    std::unordered_map<uint32_t, Actor*> m_actorsByNetId;
    uint32_t m_nextNetId;
};
```

---

### 7. Platform Socket Abstraction

**Cross-platform socket wrapper:**

```cpp
// Socket address abstraction
class WVSocketAddress {
public:
    WVSocketAddress();
    WVSocketAddress(const std::string& ip, uint16_t port);

    std::string GetIP() const;
    uint16_t GetPort() const;
    bool IsValid() const;

    // Platform-specific accessors
    #ifdef PLATFORM_WINDOWS
    const sockaddr_in& GetNative() const;
    #else
    const sockaddr_in& GetNative() const;
    #endif

private:
    sockaddr_in m_address;
};

// Socket abstraction
class WVSocket {
public:
    WVSocket();
    ~WVSocket();

    // Initialization
    bool CreateUDP();
    bool Bind(uint16_t port);
    void Close();

    // I/O (non-blocking)
    bool SetNonBlocking(bool nonBlocking);
    int32_t SendTo(const void* data, size_t size, const WVSocketAddress& dest);
    int32_t ReceiveFrom(void* buffer, size_t size, WVSocketAddress& source);

    // State
    bool IsValid() const;
    int32_t GetLastError() const;

private:
    #ifdef PLATFORM_WINDOWS
    SOCKET m_socket;
    bool m_wsaInitialized;
    #else
    int m_socket;
    #endif
};

// Socket initialization helper
class SocketSystem {
public:
    static bool Initialize();
    static void Shutdown();
};
```

---

## Packet Structure

### Base Packet Format

```
┌────────────────────────────────────────────────────┐
│  Header (12 bytes)                                 │
├────────────────────────────────────────────────────┤
│  uint32_t magic (4 bytes) - 0x57564E45 ('WVNE')   │
│  uint32_t sequence (4 bytes)                       │
│  uint16_t packetType (2 bytes)                     │
│  uint16_t payloadSize (2 bytes)                    │
├────────────────────────────────────────────────────┤
│  Payload (variable size)                           │
└────────────────────────────────────────────────────┘
```

### Packet Types

```cpp
enum class PacketType : uint16_t {
    // Connection
    ConnectionRequest = 0,
    ConnectionAccept = 1,
    ConnectionDenied = 2,
    Disconnect = 3,

    // Reliability
    Acknowledgement = 10,

    // Replication
    ActorSpawn = 20,
    ActorDestroy = 21,
    ActorReplication = 22,

    // RPC
    RPCServer = 30,
    RPCClient = 31,
    RPCMulticast = 32,

    // Control
    Heartbeat = 100,
};
```

---

## Replication Strategy

### Property Replication

1. **Registration**: Actors register properties for replication using macros or manual registration
2. **Delta Calculation**: Before sending, compare current value with last replicated value
3. **Serialization**: Only serialize changed properties (delta compression)
4. **Per-Connection State**: Track what each client has received independently

### Example Usage

```cpp
class PlayerActor : public Actor {
public:
    PlayerActor() {
        SetReplicates(true);
        RegisterReplicatedProperty("Health", &m_health, PropertyType::Int32);
        RegisterReplicatedProperty("Position", &m_position, PropertyType::Vector3);
    }

    WV_RPC_SERVER
    void TakeDamage(int32_t damage) {
        if (NetworkManager::Get().IsServer()) {
            m_health -= damage;
            if (m_health <= 0) {
                OnDeath();
            }
        }
    }

    WV_RPC_MULTICAST
    void OnDeath() {
        // Play death animation on all clients
    }

private:
    int32_t m_health = 100;
    glm::vec3 m_position;
};
```

---

## RPC System

### RPC Macros

```cpp
// Macro definitions
#define WV_RPC_SERVER    // Marks function as Server RPC
#define WV_RPC_CLIENT    // Marks function as Client RPC
#define WV_RPC_MULTICAST // Marks function as Multicast RPC

// Implementation uses template metaprogramming to:
// 1. Automatically serialize parameters
// 2. Generate network call wrapper
// 3. Register with RPCManager
```

### RPC Serialization

- Automatic serialization for primitive types (int, float, bool, etc.)
- Manual serialization for custom types via `BitStream& operator<<`
- Variadic templates for flexible parameter lists

---

## Relevancy & Interest Management

### Initial Implementation (Distance-Based)

```cpp
bool ReplicationManager::IsActorRelevantForConnection(Actor* actor, NetConnection* connection) {
    // Get the player's pawn/controlled actor for this connection
    Actor* playerActor = connection->GetPlayerActor();
    if (!playerActor) return false;

    // Always relevant if within distance
    float distance = glm::distance(actor->GetPosition(), playerActor->GetPosition());
    return distance <= m_relevancyDistance;
}
```

### Future Extensions

- **Always Relevant**: Certain actors (e.g., game state) always replicate
- **Owner Relevant**: Actor always replicates to its owner
- **Team Relevant**: Replicate based on team membership
- **Custom Relevancy**: User-defined relevancy functions

---

## Integration with WV-Core

### Minimal Changes to WV-Core

1. **Add network tick to game loop** (`src/app/App.cpp`):
```cpp
void App::Run() {
    // ... existing code ...

    while(!window.ShouldClose()) {
        // ... existing code ...

        Update();

        // Network tick (if initialized)
        if (NetworkManager::Get().IsNetworked()) {
            NetworkManager::Get().Tick(m_deltaTime);
        }

        Render();

        // ... existing code ...
    }
}
```

2. **Optional: Add World::Tick()** to update all actors if actor system is adopted

### Application Usage

```cpp
class MyGame : public WillowVox::App {
public:
    void Start() override {
        // Initialize networking
        WVNet::NetworkConfig config;
        config.mode = WVNet::NetworkMode::Server;
        config.serverPort = 7777;
        config.maxConnections = 32;
        config.tickRate = 30.0f;

        WVNet::NetworkManager::Get().Initialize(config);

        // Spawn some actors
        auto player = std::make_unique<PlayerActor>();
        WVNet::World::Get().SpawnActor(std::move(player));
    }

    void Update() override {
        // Game logic
        WVNet::World::Get().Tick(m_deltaTime);
    }
};
```

---

## Build System Integration

### CMakeLists.txt Addition

```cmake
# Add WVNet library
add_subdirectory(WVNet)

# Link to WVCore
target_link_libraries(WVCore PUBLIC WVNet)

# Platform-specific networking libraries
if(WIN32)
    target_link_libraries(WVNet PRIVATE ws2_32)
elseif(UNIX)
    # BSD sockets are part of libc on Unix
endif()
```

---

## Design Principles

1. **Minimal Coupling**: WVNet should work with WV-Core without heavy modifications
2. **Opt-in**: Networking is optional; non-networked games shouldn't include overhead
3. **Extensible**: Easy to add custom replication logic, relevancy rules, or packet types
4. **Performance**: Delta compression, relevancy culling, and efficient serialization
5. **Cross-platform**: Same API on Windows, macOS, and Linux
6. **Developer-Friendly**: Clear APIs, good defaults, easy debugging

---

## Unreal Engine Concept Mapping

| Unreal Engine | WVNet | Notes |
|---------------|-------|-------|
| `UNetDriver` | `NetDriver` | Socket management and connection handling |
| `UNetConnection` | `NetConnection` | Per-client connection state |
| `UChannel` | (Implicit in PacketType) | Simplified; may add explicit channels later |
| `AActor` | `Actor` | Replicated game object base class |
| `UWorld` | `World` | Scene/level container for actors |
| `UPROPERTY(Replicated)` | `RegisterReplicatedProperty()` | Property replication registration |
| `UFUNCTION(Server)` | `WV_RPC_SERVER` | Server RPC macro |
| `UFUNCTION(Client)` | `WV_RPC_CLIENT` | Client RPC macro |
| `UFUNCTION(NetMulticast)` | `WV_RPC_MULTICAST` | Multicast RPC macro |
| `GetLifetimeReplicatedProps()` | `GetReplicatedProperties()` | Property registration hook |
| `IsNetRelevantFor()` | `IsActorRelevantForConnection()` | Relevancy check |
| `FRepLayout` | `ActorReplicationState` | Per-connection replication state |

---

## File Structure

```
WVNet/
├── include/
│   └── wvnet/
│       ├── Core.h                    # Core definitions and macros
│       ├── NetworkManager.h
│       ├── NetDriver.h
│       ├── NetConnection.h
│       ├── ReplicationManager.h
│       ├── RPCManager.h
│       ├── Actor.h
│       ├── World.h
│       ├── Packet.h
│       ├── BitStream.h               # Serialization helper
│       └── platform/
│           ├── Socket.h              # Platform-agnostic API
│           ├── SocketWindows.h       # Windows implementation
│           └── SocketUnix.h          # Unix/macOS implementation
├── src/
│   ├── NetworkManager.cpp
│   ├── NetDriver.cpp
│   ├── NetConnection.cpp
│   ├── ReplicationManager.cpp
│   ├── RPCManager.cpp
│   ├── Actor.cpp
│   ├── World.cpp
│   ├── Packet.cpp
│   ├── BitStream.cpp
│   └── platform/
│       ├── SocketWindows.cpp
│       └── SocketUnix.cpp
├── samples/
│   ├── SimpleServer/
│   └── SimpleClient/
├── tests/
│   ├── SocketTests.cpp
│   ├── SerializationTests.cpp
│   └── ReplicationTests.cpp
├── CMakeLists.txt
└── README.md
```

---

**End of Design Document**
