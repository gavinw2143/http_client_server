#include "http_client.hpp"

namespace {

constexpr const char* http_port = "8080";

}

HttpResponse http_get(
    const std::string& host,
    const std::string& path)
{
    net::Socket sock = net::connect_tcp(host, http_port);

    std::string request =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Connection: close\r\n"
        "\r\n";

    sock.send_all(request.data(), request.size());

    std::string raw;
    char buf[4096];
    for (;;) {
        std::size_t n = sock.recv(buf, sizeof(buf));
        if (n == 0) break;
        raw.append(buf, n);
    }

    HttpResponse res;
    std::string err;
    if (!parse_http_response(raw, res, &err)) {
        throw std::runtime_error("Failed to parse HTTP response: " + err);
    }

    return res;
}
