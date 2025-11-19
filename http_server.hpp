// http_server.hpp
#pragma once

#include <cstdint>
#include "socket.hpp"
#include "http_message.hpp"

namespace net_server {

void handle_client(net::Socket client);
void run_http_server(std::uint16_t port);

} 

