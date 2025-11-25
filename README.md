# WVNet - Unreal Engine-Style Networking for WV-Core

WVNet is a comprehensive networking library that adds Unreal Engine-style server-authoritative networking and replication to the WV-Core game engine.

## Features

### ✅ Implemented Features

- **Cross-Platform Socket Abstraction**
  - Windows (WinSock2)
  - macOS and Linux (BSD sockets)
  - Non-blocking UDP communication
  - Automatic platform detection

- **Server-Authoritative Architecture**
  - Dedicated server support
  - Client-server connection management
  - Automatic connection handling and timeouts

- **Actor/Entity System**
  - Base `Actor` class for game objects
  - `World` manager for actor lifecycle
  - Automatic network ID assignment
  - Actor type registration for network spawning

- **Property Replication**
  - Delta compression (only changed properties replicate)
  - Per-connection replication state tracking
  - Support for primitive types, vectors, quaternions, and strings
  - Easy property registration API

- **RPC System**
  - Server RPCs (client→server)
  - Client RPCs (server→specific client)
  - Multicast RPCs (server→all clients)
  - Type-safe parameter serialization

- **Packet Management**
  - Reliable packet delivery with acknowledgements
  - Sequence numbering
  - Packet type system for different message types
  - Configurable send/receive buffers

- **Relevancy & Interest Management**
  - Extensible relevancy system
  - Distance-based relevancy (planned)
  - Per-connection actor visibility

## Architecture

```
NetworkManager (Singleton)
├── NetDriver (Socket & Connection Management)
│   └── NetConnection[] (Per-client state)
├── ReplicationManager (Actor Replication)
└── RPCManager (Remote Procedure Calls)

World (Singleton)
└── Actor[] (Game Objects)
    ├── Replicated Properties
    └── RPC Functions
```

## Quick Start

### 1. Include WVNet

```cpp
#include <wvnet/WVNet.h>
using namespace WVNet;
```

### 2. Initialize Networking

#### Server:
```cpp
NetworkConfig config;
config.mode = NetworkMode::Server;
config.serverPort = 7777;
config.maxConnections = 32;
config.tickRate = 30.0f;

NetworkManager::Get().Initialize(config);
```

#### Client:
```cpp
NetworkConfig config;
config.mode = NetworkMode::Client;
config.serverAddress = "127.0.0.1";
config.serverPort = 7777;

NetworkManager::Get().Initialize(config);
```

### 3. Create a Replicated Actor

```cpp
class PlayerActor : public WVNet::Actor {
public:
    PlayerActor() {
        SetReplicates(true);

        // Register properties for replication
        RegisterProperty("Health", &m_health);
        RegisterProperty("Position", &m_position);
    }

    std::string GetTypeName() const override {
        return "PlayerActor";
    }

    void Tick(float deltaTime) override {
        // Game logic here
    }

    void OnReplicated() override {
        // Called when properties are updated from server
    }

private:
    int32_t m_health = 100;
    glm::vec3 m_position{0.0f};
};
```

### 4. Register Actor Types

```cpp
// Both server and client must register the same types
World::Get().RegisterActorType<PlayerActor>("PlayerActor");
```

### 5. Spawn Actors (Server-side)

```cpp
PlayerActor* player = World::Get().SpawnActor<PlayerActor>();
// Actor will automatically replicate to connected clients
```

### 6. Game Loop Integration

```cpp
void Update(float deltaTime) {
    // Update world (ticks all actors)
    World::Get().Tick(deltaTime);

    // Process networking
    NetworkManager::Get().Tick(deltaTime);

    // Your game logic here
}
```

### 7. Shutdown

```cpp
NetworkManager::Get().Shutdown();
```

## Using RPCs

### Defining RPCs

```cpp
class PlayerActor : public Actor {
public:
    // Server RPC: Called on client, executed on server
    WV_RPC_SERVER
    void TakeDamage(int32_t damage) {
        if (NetworkManager::Get().IsServer()) {
            m_health -= damage;
        }
    }

    // Client RPC: Called on server, executed on specific client
    WV_RPC_CLIENT
    void ShowMessage(const std::string& message) {
        // Display message on client
    }

    // Multicast RPC: Called on server, executed on all clients
    WV_RPC_MULTICAST
    void PlayDeathAnimation() {
        // Play animation on all clients
    }
};
```

### Calling RPCs

```cpp
// On client, call server RPC
BitStream params;
params.WriteInt32(25);
NetworkManager::Get().GetRPCManager()->CallServerRPC(
    playerActor, "TakeDamage", params
);

// On server, call client RPC
BitStream params;
params.WriteString("You took damage!");
NetworkManager::Get().GetRPCManager()->CallClientRPC(
    playerActor, clientConnection, "ShowMessage", params
);

// On server, call multicast RPC
BitStream params;
NetworkManager::Get().GetRPCManager()->CallMulticastRPC(
    playerActor, "PlayDeathAnimation", params
);
```

## Configuration Options

### NetworkConfig

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `mode` | `NetworkMode` | `Standalone` | Server, Client, or Standalone |
| `serverAddress` | `std::string` | `"127.0.0.1"` | Server IP address (client only) |
| `serverPort` | `uint16_t` | `7777` | Server port |
| `maxConnections` | `uint32_t` | `64` | Maximum clients (server only) |
| `tickRate` | `float` | `30.0f` | Network update rate (Hz) |
| `enableRelevancy` | `bool` | `false` | Enable actor relevancy checks |
| `relevancyDistance` | `float` | `10000.0f` | Distance for actor relevancy |

## Building

### Requirements
- CMake 3.24+
- C++20 compatible compiler
- GLM (automatically fetched)

### Build Steps

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Running Samples

```bash
# Terminal 1: Run server
./build/WVNet/samples/SimpleServer/SimpleServer

# Terminal 2: Run client
./build/WVNet/samples/SimpleClient/SimpleClient
```

## Architecture Details

### Packet Structure

All packets follow this structure:

```
Header (12 bytes):
  - magic (4 bytes): 0x57564E45 ('WVNE')
  - sequence (4 bytes): Sequence number
  - packetType (2 bytes): Type identifier
  - payloadSize (2 bytes): Size of payload

Payload (variable):
  - Type-specific data
```

### Packet Types

- **Connection**: ConnectionRequest, ConnectionAccept, ConnectionDenied, Disconnect
- **Reliability**: Acknowledgement, Heartbeat
- **Replication**: ActorSpawn, ActorDestroy, ActorReplication
- **RPC**: RPCServer, RPCClient, RPCMulticast

### Property Types Supported

- Primitives: bool, int8/16/32/64, uint8/16/32/64, float, double
- Math: glm::vec3, glm::quat
- String: std::string
- Custom: Implement serialization for your types

### Replication Strategy

1. **Registration**: Actors register properties for replication
2. **Delta Calculation**: Only changed properties are sent
3. **Per-Connection State**: Each client has independent replication state
4. **Reliable Delivery**: Replication packets are sent reliably

## Unreal Engine Concept Mapping

| Unreal Engine | WVNet | Notes |
|---------------|-------|-------|
| `UNetDriver` | `NetDriver` | Socket and connection management |
| `UNetConnection` | `NetConnection` | Per-client connection state |
| `AActor` | `Actor` | Replicated game object base class |
| `UWorld` | `World` | Scene/level container |
| `UPROPERTY(Replicated)` | `RegisterProperty()` | Property replication |
| `UFUNCTION(Server)` | `WV_RPC_SERVER` | Server RPC |
| `UFUNCTION(Client)` | `WV_RPC_CLIENT` | Client RPC |
| `UFUNCTION(NetMulticast)` | `WV_RPC_MULTICAST` | Multicast RPC |

## Examples

See `WVNet/samples/` for complete working examples:

- **SimpleServer**: Basic server that spawns actors and replicates to clients
- **SimpleClient**: Basic client that connects and receives replicated actors

## Future Enhancements

- [ ] Advanced relevancy system (distance-based, team-based)
- [ ] Compression (Huffman encoding, delta compression)
- [ ] Interpolation and prediction
- [ ] Voice chat support
- [ ] NAT traversal
- [ ] Encryption (TLS/DTLS)
- [ ] Bandwidth throttling
- [ ] Statistics and profiling
- [ ] Connection migration
- [ ] Better RPC macro system with automatic parameter serialization

## Integration with WV-Core

WVNet is designed as a standalone library that can be optionally used with WV-Core:

1. Link WVNet to your WV-Core application
2. Initialize networking in your `App::Start()`
3. Call `NetworkManager::Get().Tick()` in your `App::Update()`
4. Spawn actors using `World::Get().SpawnActor()`

Example:

```cpp
class MyGame : public WillowVox::App {
public:
    void Start() override {
        // Initialize networking
        WVNet::NetworkConfig config;
        config.mode = WVNet::NetworkMode::Server;
        WVNet::NetworkManager::Get().Initialize(config);

        // Spawn actors
        WVNet::World::Get().RegisterActorType<PlayerActor>("PlayerActor");
        WVNet::World::Get().SpawnActor<PlayerActor>();
    }

    void Update() override {
        // Tick networking and world
        WVNet::World::Get().Tick(m_deltaTime);
        WVNet::NetworkManager::Get().Tick(m_deltaTime);
    }
};
```

## License

Same as WV-Core.

## Credits

Designed and implemented as a networking layer for the WV game engine, inspired by Unreal Engine's networking model.
