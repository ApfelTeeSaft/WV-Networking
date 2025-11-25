#pragma once

#include <wvnet/Core.h>
#include <string>
#include <cstring>

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

namespace WVNet {

    // Forward declarations
    class WVSocket;
    class WVSocketAddress;

    //=============================================================================
    // SocketSystem - Platform initialization/shutdown
    //=============================================================================

    class SocketSystem {
    public:
        static bool Initialize();
        static void Shutdown();
        static bool IsInitialized();

    private:
        static bool s_initialized;
    };

    //=============================================================================
    // WVSocketAddress - Cross-platform socket address abstraction
    //=============================================================================

    class WVSocketAddress {
    public:
        WVSocketAddress();
        WVSocketAddress(const std::string& ip, uint16_t port);
        WVSocketAddress(const sockaddr_in& addr);

        // Accessors
        std::string GetIP() const;
        uint16_t GetPort() const;
        bool IsValid() const;

        // Comparison
        bool operator==(const WVSocketAddress& other) const;
        bool operator!=(const WVSocketAddress& other) const;

        // Platform-specific accessor
        const sockaddr_in& GetNative() const { return m_address; }
        sockaddr_in& GetNative() { return m_address; }

        // Utilities
        std::string ToString() const;

    private:
        sockaddr_in m_address;
        bool m_isValid;
    };

    //=============================================================================
    // WVSocket - Cross-platform UDP socket abstraction
    //=============================================================================

    class WVSocket {
    public:
        WVSocket();
        ~WVSocket();

        // No copy
        WVSocket(const WVSocket&) = delete;
        WVSocket& operator=(const WVSocket&) = delete;

        // Move allowed
        WVSocket(WVSocket&& other) noexcept;
        WVSocket& operator=(WVSocket&& other) noexcept;

        // Initialization
        bool CreateUDP();
        bool Bind(uint16_t port);
        void Close();

        // Configuration
        bool SetNonBlocking(bool nonBlocking);
        bool SetReuseAddress(bool reuse);
        bool SetReceiveBufferSize(int32_t size);
        bool SetSendBufferSize(int32_t size);

        // I/O
        int32_t SendTo(const void* data, size_t size, const WVSocketAddress& dest);
        int32_t ReceiveFrom(void* buffer, size_t size, WVSocketAddress& source);

        // State
        bool IsValid() const;
        int32_t GetLastError() const;
        std::string GetErrorString() const;
        uint16_t GetBoundPort() const;

    private:
        void SetError(int32_t error);

        #ifdef PLATFORM_WINDOWS
        SOCKET m_socket;
        static constexpr SOCKET INVALID_SOCKET_VALUE = INVALID_SOCKET;
        #else
        int m_socket;
        static constexpr int INVALID_SOCKET_VALUE = -1;
        #endif

        int32_t m_lastError;
        uint16_t m_boundPort;
    };

} // namespace WVNet
