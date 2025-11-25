#include <wvnet/platform/Socket.h>
#include <cstdio>

namespace WVNet {

    //=============================================================================
    // SocketSystem Implementation
    //=============================================================================

    bool SocketSystem::s_initialized = false;

    bool SocketSystem::Initialize() {
        if (s_initialized) {
            return true;
        }

        #ifdef PLATFORM_WINDOWS
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            WVNET_LOG_FMT("WSAStartup failed with error: %d", result);
            return false;
        }
        #endif

        s_initialized = true;
        WVNET_LOG("Socket system initialized");
        return true;
    }

    void SocketSystem::Shutdown() {
        if (!s_initialized) {
            return;
        }

        #ifdef PLATFORM_WINDOWS
        WSACleanup();
        #endif

        s_initialized = false;
        WVNET_LOG("Socket system shutdown");
    }

    bool SocketSystem::IsInitialized() {
        return s_initialized;
    }

    //=============================================================================
    // WVSocketAddress Implementation
    //=============================================================================

    WVSocketAddress::WVSocketAddress() : m_isValid(false) {
        memset(&m_address, 0, sizeof(m_address));
    }

    WVSocketAddress::WVSocketAddress(const std::string& ip, uint16_t port) : m_isValid(false) {
        memset(&m_address, 0, sizeof(m_address));
        m_address.sin_family = AF_INET;
        m_address.sin_port = htons(port);

        // Parse IP address
        if (ip.empty() || ip == "0.0.0.0") {
            m_address.sin_addr.s_addr = INADDR_ANY;
            m_isValid = true;
        } else {
            #ifdef PLATFORM_WINDOWS
            if (inet_pton(AF_INET, ip.c_str(), &m_address.sin_addr) == 1) {
                m_isValid = true;
            }
            #else
            if (inet_pton(AF_INET, ip.c_str(), &m_address.sin_addr) == 1) {
                m_isValid = true;
            }
            #endif
        }
    }

    WVSocketAddress::WVSocketAddress(const sockaddr_in& addr)
        : m_address(addr), m_isValid(true) {
    }

    std::string WVSocketAddress::GetIP() const {
        char buffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &m_address.sin_addr, buffer, INET_ADDRSTRLEN);
        return std::string(buffer);
    }

    uint16_t WVSocketAddress::GetPort() const {
        return ntohs(m_address.sin_port);
    }

    bool WVSocketAddress::IsValid() const {
        return m_isValid;
    }

    bool WVSocketAddress::operator==(const WVSocketAddress& other) const {
        return m_address.sin_addr.s_addr == other.m_address.sin_addr.s_addr &&
               m_address.sin_port == other.m_address.sin_port;
    }

    bool WVSocketAddress::operator!=(const WVSocketAddress& other) const {
        return !(*this == other);
    }

    std::string WVSocketAddress::ToString() const {
        if (!m_isValid) {
            return "Invalid";
        }
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s:%u", GetIP().c_str(), GetPort());
        return std::string(buffer);
    }

    //=============================================================================
    // WVSocket Implementation
    //=============================================================================

    WVSocket::WVSocket()
        : m_socket(INVALID_SOCKET_VALUE), m_lastError(0), m_boundPort(0) {
    }

    WVSocket::~WVSocket() {
        Close();
    }

    WVSocket::WVSocket(WVSocket&& other) noexcept
        : m_socket(other.m_socket), m_lastError(other.m_lastError), m_boundPort(other.m_boundPort) {
        other.m_socket = INVALID_SOCKET_VALUE;
        other.m_lastError = 0;
        other.m_boundPort = 0;
    }

    WVSocket& WVSocket::operator=(WVSocket&& other) noexcept {
        if (this != &other) {
            Close();
            m_socket = other.m_socket;
            m_lastError = other.m_lastError;
            m_boundPort = other.m_boundPort;
            other.m_socket = INVALID_SOCKET_VALUE;
            other.m_lastError = 0;
            other.m_boundPort = 0;
        }
        return *this;
    }

    bool WVSocket::CreateUDP() {
        if (!SocketSystem::IsInitialized()) {
            WVNET_LOG_ERROR("Socket system not initialized");
            return false;
        }

        if (IsValid()) {
            Close();
        }

        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        #ifdef PLATFORM_WINDOWS
        if (m_socket == INVALID_SOCKET) {
            SetError(WSAGetLastError());
            WVNET_LOG_FMT("Failed to create socket: %s", GetErrorString().c_str());
            return false;
        }
        #else
        if (m_socket < 0) {
            SetError(errno);
            WVNET_LOG_FMT("Failed to create socket: %s", GetErrorString().c_str());
            return false;
        }
        #endif

        return true;
    }

    bool WVSocket::Bind(uint16_t port) {
        if (!IsValid()) {
            WVNET_LOG_ERROR("Cannot bind invalid socket");
            return false;
        }

        WVSocketAddress bindAddr("0.0.0.0", port);
        sockaddr_in addr = bindAddr.GetNative();

        if (bind(m_socket, (sockaddr*)&addr, sizeof(addr)) != 0) {
            #ifdef PLATFORM_WINDOWS
            SetError(WSAGetLastError());
            #else
            SetError(errno);
            #endif
            WVNET_LOG_FMT("Failed to bind socket to port %u: %s", port, GetErrorString().c_str());
            return false;
        }

        m_boundPort = port;
        WVNET_LOG_FMT("Socket bound to port %u", port);
        return true;
    }

    void WVSocket::Close() {
        if (IsValid()) {
            #ifdef PLATFORM_WINDOWS
            closesocket(m_socket);
            #else
            close(m_socket);
            #endif
            m_socket = INVALID_SOCKET_VALUE;
            m_boundPort = 0;
        }
    }

    bool WVSocket::SetNonBlocking(bool nonBlocking) {
        if (!IsValid()) {
            return false;
        }

        #ifdef PLATFORM_WINDOWS
        u_long mode = nonBlocking ? 1 : 0;
        if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
            SetError(WSAGetLastError());
            return false;
        }
        #else
        int flags = fcntl(m_socket, F_GETFL, 0);
        if (flags < 0) {
            SetError(errno);
            return false;
        }

        flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        if (fcntl(m_socket, F_SETFL, flags) < 0) {
            SetError(errno);
            return false;
        }
        #endif

        return true;
    }

    bool WVSocket::SetReuseAddress(bool reuse) {
        if (!IsValid()) {
            return false;
        }

        int optval = reuse ? 1 : 0;
        if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,
                       (const char*)&optval, sizeof(optval)) != 0) {
            #ifdef PLATFORM_WINDOWS
            SetError(WSAGetLastError());
            #else
            SetError(errno);
            #endif
            return false;
        }

        return true;
    }

    bool WVSocket::SetReceiveBufferSize(int32_t size) {
        if (!IsValid()) {
            return false;
        }

        if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF,
                       (const char*)&size, sizeof(size)) != 0) {
            #ifdef PLATFORM_WINDOWS
            SetError(WSAGetLastError());
            #else
            SetError(errno);
            #endif
            return false;
        }

        return true;
    }

    bool WVSocket::SetSendBufferSize(int32_t size) {
        if (!IsValid()) {
            return false;
        }

        if (setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF,
                       (const char*)&size, sizeof(size)) != 0) {
            #ifdef PLATFORM_WINDOWS
            SetError(WSAGetLastError());
            #else
            SetError(errno);
            #endif
            return false;
        }

        return true;
    }

    int32_t WVSocket::SendTo(const void* data, size_t size, const WVSocketAddress& dest) {
        if (!IsValid() || !dest.IsValid()) {
            return -1;
        }

        const sockaddr_in& addr = dest.GetNative();
        int32_t bytesSent = sendto(m_socket, (const char*)data, (int)size, 0,
                                    (const sockaddr*)&addr, sizeof(addr));

        if (bytesSent < 0) {
            #ifdef PLATFORM_WINDOWS
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                SetError(error);
            }
            #else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                SetError(errno);
            }
            #endif
        }

        return bytesSent;
    }

    int32_t WVSocket::ReceiveFrom(void* buffer, size_t size, WVSocketAddress& source) {
        if (!IsValid()) {
            return -1;
        }

        sockaddr_in from;
        socklen_t fromLen = sizeof(from);

        int32_t bytesReceived = recvfrom(m_socket, (char*)buffer, (int)size, 0,
                                         (sockaddr*)&from, &fromLen);

        if (bytesReceived < 0) {
            #ifdef PLATFORM_WINDOWS
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                SetError(error);
            }
            #else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                SetError(errno);
            }
            #endif
        } else if (bytesReceived > 0) {
            source = WVSocketAddress(from);
        }

        return bytesReceived;
    }

    bool WVSocket::IsValid() const {
        return m_socket != INVALID_SOCKET_VALUE;
    }

    int32_t WVSocket::GetLastError() const {
        return m_lastError;
    }

    std::string WVSocket::GetErrorString() const {
        #ifdef PLATFORM_WINDOWS
        char buffer[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      nullptr, m_lastError, 0, buffer, sizeof(buffer), nullptr);
        return std::string(buffer);
        #else
        return std::string(strerror(m_lastError));
        #endif
    }

    uint16_t WVSocket::GetBoundPort() const {
        return m_boundPort;
    }

    void WVSocket::SetError(int32_t error) {
        m_lastError = error;
    }

} // namespace WVNet
