#pragma once

#include <cstdint>
#include "socket.hpp"
#include "http_message.hpp"

namespace net_server {

void handle_client(net::Socket client);
void run_http_server(std::uint16_t port);
bool serve_static(const std::string& doc_root,
                  const HttpRequest& req,
                  HttpResponse& res);

} 

