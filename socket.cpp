#include "socket.hpp"
#include <stdexcept>
#include <system_error>

namespace net {

/* ---
 * Setup
*/

Socket::Socket() noexcept
    : handle_(INVALID_SOCKET_HANDLE) {}

Socket::~Socket() {
    reset();
}

Socket::Socket(Socket&& other) noexcept
    : handle_(other.handle_) {
    other.handle_ = INVALID_SOCKET_HANDLE;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        reset();
        handle_ = other.handle_;
        other.handle_ = INVALID_SOCKET_HANDLE;
    }
    return *this;
}

Socket Socket::tcp_v4() {
    SocketHandle h = ::socket(AF_INET, SOCK_STREAM, 0);
    if (h == INVALID_SOCKET_HANDLE) {
        throw std::runtime_error("socket(AF_INET, SOCK_STREAM) failed");
    }
    return Socket(h);
}

/* ---
 * Client methods
*/

void Socket::connect_v4(const std::string& ip, uint16_t port) {
    if (!valid()) {
        throw std::runtime_error("connect_v4 called on invalid socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // inet_pton available on Windows thru <ws2tcpip.h>
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        throw std::runtime_error("inet_pton failed for IP: " + ip);
    }

    if (::connect(handle_,
                  reinterpret_cast<sockaddr*>(&addr),
                  sizeof(addr)) < 0) {
        throw std::runtime_error("connect failed");
    }
}

/* ---
 * Server methods
*/

void Socket::bind_v4_any(uint16_t port) {
    if (!valid()) {
        throw std::runtime_error("bind_v4_any called on invalid socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    ::setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<const char*>(&opt),
                 sizeof(opt));

    if (::bind(handle_,
               reinterpret_cast<sockaddr*>(&addr),
               sizeof(addr)) < 0) {
        throw std::runtime_error("bind failed");
    }
}

void Socket::listen(int backlog) {
    if (!valid()) {
        throw std::runtime_error("listen called on invalid socket");
    }
    if (::listen(handle_, backlog) < 0) {
        throw std::runtime_error("listen failed");
    }
}

Socket Socket::accept() {
    if (!valid()) {
        throw std::runtime_error("accept called on invalid socket");
    }

    sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);

    SocketHandle client =
        ::accept(handle_, reinterpret_cast<sockaddr*>(&addr), &addrlen);

    if (client == INVALID_SOCKET_HANDLE) {
        throw std::runtime_error("accept failed");
    }

    return Socket(client); // move-constructed, RAII-managed
}

/* ---
 * I/O
*/

std::size_t Socket::send(const void* buffer, std::size_t len) {
    if (!valid()) {
        throw std::runtime_error("send on invalid socket");
    }

    // Note: narrowing to int: fine for small HTTP payloads
    int sent = ::send(handle_,
                      static_cast<const char*>(buffer),
                      static_cast<int>(len),
                      0);
    if (sent < 0) {
        throw std::runtime_error("send failed");
    }
    return static_cast<std::size_t>(sent);
}

std::size_t Socket::recv(void* buffer, std::size_t max_len) {
    if (!valid()) {
        throw std::runtime_error("recv on invalid socket");
    }

    int received = ::recv(handle_,
                          static_cast<char*>(buffer),
                          static_cast<int>(max_len),
                          0);
    if (received < 0) {
        throw std::runtime_error("recv failed");
    }
    return static_cast<std::size_t>(received);
}

// send_all: loop until all bytes are sent or error
void Socket::send_all(const void* buffer, std::size_t len) {
    const char* data = static_cast<const char*>(buffer);
    std::size_t total_sent = 0;

    while (total_sent < len) {
        std::size_t n = send(data + total_sent, len - total_sent);
        if (n == 0) {
            throw std::runtime_error("send_all: connection closed while sending");
        }
        total_sent += n;
    }
}

/* ---
 * Helpers
*/

bool Socket::valid() const noexcept {
    return handle_ != INVALID_SOCKET_HANDLE;
}

SocketHandle Socket::native_handle() const noexcept {
    return handle_;
}

void Socket::reset(SocketHandle new_handle) noexcept {
    if (handle_ != INVALID_SOCKET_HANDLE) {
        close_socket(handle_);
    }
    handle_ = new_handle;
}


} // namespace net
