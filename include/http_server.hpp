#pragma once

#include <cstdint>
#include "socket.hpp"
#include "net_tls.hpp"
#include "http_message.hpp"

namespace net_server {

void run_http_server(std::uint16_t port);
void handle_client(net::Socket client);
void handle_client_tls(net::TlsSocket client);
bool serve_static(const std::string& doc_root,
                  const HttpRequest& req,
                  HttpResponse& res);

} 

