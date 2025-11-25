#include <wvnet/WVNet.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace WVNet;

// Example replicated actor (same as server)
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

    void OnSpawn() override {
        std::cout << "[Client] PlayerActor spawned with NetID: " << GetNetId() << std::endl;
    }

    void OnReplicated() override {
        // Called when properties are updated from server
        std::cout << "[Client] PlayerActor replicated - Health: " << m_health
                  << " | Position: (" << m_position.x << ", " << m_position.y << ", " << m_position.z << ")"
                  << std::endl;
    }

    void OnDestroy() override {
        std::cout << "[Client] PlayerActor destroyed" << std::endl;
    }

    int32_t GetHealth() const { return m_health; }
    const glm::vec3& GetPos() const { return m_position; }

private:
    int32_t m_health = 100;
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
};

int main() {
    std::cout << "=== WVNet Simple Client ===" << std::endl;

    // Initialize networking
    NetworkConfig config;
    config.mode = NetworkMode::Client;
    config.serverAddress = "127.0.0.1";
    config.serverPort = 7777;
    config.tickRate = 30.0f;

    if (!NetworkManager::Get().Initialize(config)) {
        std::cerr << "Failed to initialize networking" << std::endl;
        return 1;
    }

    std::cout << "Connecting to server at 127.0.0.1:7777..." << std::endl;

    // Register actor types that can be spawned
    World::Get().RegisterActorType<PlayerActor>("PlayerActor");

    // Main client loop
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
            std::cout << "[Client] Tick - Active actors: " << World::Get().GetActors().size() << std::endl;

            // Print info about replicated actors
            for (Actor* actor : World::Get().GetActors()) {
                if (PlayerActor* player = dynamic_cast<PlayerActor*>(actor)) {
                    std::cout << "  Player - Health: " << player->GetHealth()
                              << " | Pos: (" << player->GetPos().x << ", "
                              << player->GetPos().y << ", " << player->GetPos().z << ")"
                              << std::endl;
                }
            }
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

    std::cout << "Disconnecting from server..." << std::endl;
    NetworkManager::Get().Shutdown();

    return 0;
}
