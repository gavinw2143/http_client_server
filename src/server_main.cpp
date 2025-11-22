#include <iostream>
#include "net_platform.hpp"
#include "http_server.hpp"

int main() {
    try {
        net::SocketSystem sys;

        net_server::run_http_server(8443);

    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}

