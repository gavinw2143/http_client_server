#include "net_platform.hpp"

namespace net {

SocketSystem::SocketSystem() {
#ifdef _WIN32
    WSADATA data{};
    int res = ::WSAStartup(MAKEWORD(2, 2), &data);
    if (res != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif
}

SocketSystem::~SocketSystem() {
#ifdef _WIN32
    ::WSACleanup();
#endif
}

int close_socket(SocketHandle s) {
#ifdef _WIN32
    return ::closesocket(s);
#else
    return ::close(s);
#endif
}

} 
