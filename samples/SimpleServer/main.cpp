#include <wvnet/WVNet.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace WVNet;

// Example replicated actor
class PlayerActor : public Actor {
public:
    PlayerActor() {
        SetReplicates(true);

        // Register replicated properties
        RegisterProperty("Health", &m_health);
        RegisterProperty("Position", &m_position);
    }

    std::string GetTypeName() const override {
        return "PlayerActor";
    }

    void Tick(float deltaTime) override {
        // Example: Move in a circle
        static float time = 0.0f;
        time += deltaTime;

        m_position.x = std::cos(time) * 5.0f;
        m_position.z = std::sin(time) * 5.0f;

        // Example: Decrease health over time (for demonstration)
        if (m_health > 0) {
            m_health -= static_cast<int32_t>(deltaTime * 10.0f);
            if (m_health < 0) m_health = 0;
        }
    }

    void OnSpawn() override {
        std::cout << "[Server] PlayerActor spawned with NetID: " << GetNetId() << std::endl;
    }

    void OnDestroy() override {
        std::cout << "[Server] PlayerActor destroyed" << std::endl;
    }

private:
    int32_t m_health = 100;
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
};

int main() {
    std::cout << "=== WVNet Simple Server ===" << std::endl;

    // Initialize networking
    NetworkConfig config;
    config.mode = NetworkMode::Server;
    config.serverPort = 7777;
    config.maxConnections = 10;
    config.tickRate = 30.0f;

    if (!NetworkManager::Get().Initialize(config)) {
        std::cerr << "Failed to initialize networking" << std::endl;
        return 1;
    }

    std::cout << "Server started on port 7777" << std::endl;
    std::cout << "Waiting for clients..." << std::endl;

    // Register actor types that can be spawned
    World::Get().RegisterActorType<PlayerActor>("PlayerActor");

    // Spawn a test player actor
    PlayerActor* player = World::Get().SpawnActor<PlayerActor>();
    std::cout << "Spawned test player actor" << std::endl;

    // Main server loop
    const float targetFrameTime = 1.0f / 60.0f; // 60 FPS
    auto lastTime = std::chrono::high_resolution_clock::now();

    bool running = true;
    int frameCount = 0;

    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Tick world (actors)
        World::Get().Tick(deltaTime);

        // Tick networking
        NetworkManager::Get().Tick(deltaTime);

        frameCount++;
        if (frameCount % 300 == 0) {  // Every 5 seconds at 60 FPS
            std::cout << "[Server] Tick - Active connections: "
                      << NetworkManager::Get().GetNetDriver()->GetConnections().size()
                      << " | Player Health: " << player->GetTypeName()
                      << std::endl;
        }

        // Sleep to maintain frame rate
        auto frameEnd = std::chrono::high_resolution_clock::now();
        float frameDuration = std::chrono::duration<float>(frameEnd - currentTime).count();
        if (frameDuration < targetFrameTime) {
            std::this_thread::sleep_for(
                std::chrono::duration<float>(targetFrameTime - frameDuration)
            );
        }

        // Simple exit condition (Ctrl+C will work better)
        if (frameCount > 18000) { // Run for 5 minutes at 60 FPS
            running = false;
        }
    }

    std::cout << "Shutting down server..." << std::endl;
    NetworkManager::Get().Shutdown();

    return 0;
}
