#include <iostream>
#include <string>

#include "net_platform.hpp"
#include "http_client.hpp"
#include "http_message.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <path>\n";
        std::cerr << "Example: " << argv[0] << " example.com /\n";
        return 1;
    }

    std::string host = argv[1];
    std::string path = argv[2];

    try {
        net::SocketSystem sys;  // init sockets (RAII)

        HttpResponse res = http_get(host, path);

        std::cout << "Status: " << res.status_code
                  << " " << res.reason << "\n";

        std::cout << "Headers:\n";
        for (const auto& h : res.headers) {
            std::cout << h.name << ": " << h.value << "\n";
        }

        std::cout << "\nBody:\n";
        std::cout << res.body << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
