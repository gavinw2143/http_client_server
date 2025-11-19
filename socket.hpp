#pragma once
#include "net_platform.hpp"
#include <cstddef>

namespace net {

class Socket {
public:
    Socket() noexcept;
    explicit Socket(SocketHandle handle) noexcept;

    ~Socket();

    // No copy construction/assignment
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Move construction/assignment permitted
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    static Socket tcp_v4(); // create IPv4 socket

    // Client side
    void connect_v4(const std::string& ip, uint16_t port);

    // Server side
    void bind_v4_any(uint16_t port);
    void listen(int backlog = 16);
    Socket accept();

    // I/O
    std::size_t send(const void* buffer, std::size_t len);
    std::size_t recv(void* buffer, std::size_t max_len);
    void send_all(const void* buffer, std::size_t len);

    // Helpers
    bool valid() const noexcept;
    SocketHandle native_handle() const noexcept;
    void reset(SocketHandle new_handle = INVALID_SOCKET_HANDLE) noexcept;

private:
    SocketHandle handle_;
};

} // namespace net
