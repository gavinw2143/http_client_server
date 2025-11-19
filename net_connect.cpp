#include "net_connect.hpp"
#include "net_platform.hpp"

namespace net {

Socket connect_tcp(const std::string& host,
                   const std::string& service)
{
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;   // allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP; // explicitly TCP

    addrinfo* results = nullptr;
    int rc = ::getaddrinfo(host.c_str(), service.c_str(),
                           &hints, &results);
    if (rc != 0) {
        throw std::runtime_error(std::string("getaddrinfo failed: ")
                                 + gai_strerror(rc));
    }

    Socket sock; // starts invalid
    for (addrinfo* ai = results; ai != nullptr; ai = ai->ai_next) {
        // Create a socket for this address
        SocketHandle h = ::socket(ai->ai_family,
                                  ai->ai_socktype,
                                  ai->ai_protocol);
        if (h == INVALID_SOCKET_HANDLE) {
            continue; // try next addrinfo
        }

        if (::connect(h, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) == 0) {
            // success: move into our RAII wrapper
            sock = Socket(h); 
            break;
        }

        // connect failed; close this handle and try next
        close_socket(h);
    }

    ::freeaddrinfo(results);

    if (!sock.valid()) {
        throw std::runtime_error("connect_tcp: could not connect to any address");
    }

    return sock; // move-return
}

} // namespace net
