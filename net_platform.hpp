#pragma once

#include <stdexcept>

#ifdef _WIN32
  #define NOMINMAX
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using SocketHandle = SOCKET;
  inline constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else 
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <netdb.h>
  using SocketHandle = int;
  inline constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
#endif

namespace net {

struct SocketSystem {
  SocketSystem();
  ~SocketSystem();
};

int close_socket(SocketHandle sock);

}
