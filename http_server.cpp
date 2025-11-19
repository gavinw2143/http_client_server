#include "http_server.hpp"
#include <iostream>

void net_server::handle_client(net::Socket client) {
    std::string raw;
    char buf[4096];

    for (;;) {
        std::size_t n = client.recv(buf, sizeof(buf));
        // Client closed
        if (n == 0) {
            break;
        }
        raw.append(buf, n);
        // End of headers
        if (raw.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    HttpRequest req;
    std::string error;
    if (!parse_http_request(raw, req, &error)) {
        return;
    }

    HttpResponse res;

    if (req.method != "GET") {
        res.status_code = 405;
        res.reason = "Method Not Allowed";
        res.body = "Only GET supported\n";
    } else if (req.target == "/") {
        res.body = "Root page\n";
    } else if (req.target == "/hello") {
        res.body = "Hello endpoint\n";
    } else {
        res.status_code = 404;
        res.reason = "Not Found";
        res.body = "404 Not Found\n";
    }

    res.set_header("Content-Type", "text/plain");
    res.set_header("Content-Length", std::to_string(res.body.size()));
    res.set_header("Connection", "close");

    std::string response_str = serialize_http_response(res);
    client.send_all(response_str.data(), response_str.size());
}

void net_server::run_http_server(uint16_t port) {
    net::Socket listener = net::Socket::tcp_v4();
    listener.bind_v4_any(port);
    listener.listen();

    std::cout << "Listening on port " << port << "...\n";

    for (;;) {
        std::cout << "Waiting for connection...\n";
        net::Socket client = listener.accept();
        std::cout << "Accepted connection!\n";

        // For now: handle one client at a time, blocking
        handle_client(std::move(client));
    }
}
