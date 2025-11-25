#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

// Platform detection (should match WV-Core's definitions)
#if defined(_WIN32) || defined(_WIN64)
    #ifndef PLATFORM_WINDOWS
        #define PLATFORM_WINDOWS
    #endif
#elif defined(__APPLE__) || defined(__MACH__)
    #ifndef PLATFORM_MACOS
        #define PLATFORM_MACOS
    #endif
    #define PLATFORM_UNIX
#elif defined(__linux__)
    #ifndef PLATFORM_LINUX
        #define PLATFORM_LINUX
    #endif
    #define PLATFORM_UNIX
#elif defined(__unix__)
    #ifndef PLATFORM_UNIX
        #define PLATFORM_UNIX
    #endif
#endif

// Networking constants
namespace WVNet {

    constexpr uint32_t PACKET_MAGIC = 0x57564E45; // 'WVNE'
    constexpr uint16_t DEFAULT_SERVER_PORT = 7777;
    constexpr uint32_t DEFAULT_MAX_CONNECTIONS = 64;
    constexpr float DEFAULT_TICK_RATE = 30.0f;
    constexpr float DEFAULT_RELEVANCY_DISTANCE = 10000.0f;
    constexpr size_t MAX_PACKET_SIZE = 1024; // 1 KB for now, can increase later

    // Network modes
    enum class NetworkMode {
        Standalone,  // No networking
        Server,      // Dedicated or listen server
        Client       // Client connecting to server
    };

    // Logging helper (can be replaced with WV-Core's logger)
    #define WVNET_LOG(msg) printf("[WVNet] %s\n", msg)
    #define WVNET_LOG_ERROR(msg) fprintf(stderr, "[WVNet ERROR] %s\n", msg)
    #define WVNET_LOG_FMT(fmt, ...) printf("[WVNet] " fmt "\n", __VA_ARGS__)

} // namespace WVNet
