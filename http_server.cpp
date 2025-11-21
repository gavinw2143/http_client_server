#include <iostream>
#include <fstream>
#include "http_server.hpp"

void net_server::handle_client(net::Socket client) {
    std::string raw;
    char buf[4096];
    std::size_t header_end = std::string::npos;

    for (;;) {
        std::size_t n = client.recv(buf, sizeof(buf));
        // Client closed
        if (n == 0) {
            break;
        }
        raw.append(buf, n);

        auto pos = raw.find("\r\n\r\n");
        // End of headers
        if (pos != std::string::npos) {
            header_end = pos + 4; // +4 -> index after \r\n\r\n
            break;
        }
    }
    if (header_end == std::string::npos) {
        // never saw end-of-headers → malformed / too large / etc
        return;
    }

    HttpRequest req;
    std::string error;
    std::string head = raw.substr(0, header_end);
    if (!parse_http_request(head, req, &error)) {
        return;
    }

    std::size_t content_length = 0;
    if (auto len_str = req.header_value("Content-Length")) {
        content_length = static_cast<std::size_t>(std::stoul(*len_str));
    }

    req.body = raw.substr(header_end);
    std::size_t received = req.body.size();

    while (received < content_length) {
        std::size_t to_read = content_length - received;
        if (to_read > sizeof(buf)) {
            to_read = sizeof(buf);
        }

        std::size_t n = client.recv(buf, to_read);
        if (n == 0) {
            // client closed early; you can decide how to handle this
            break;
        }

        req.body.append(buf, n);
        received += n;
    }

    HttpResponse res;

    if (req.method == "GET") {
        // try static files
        if (!serve_static("./www", req, res)) {
            // temp fallback
            if (req.target == "/hello") {
                res.body = "Hello endpoint\n";
            } else {
                res.status_code = 404;
                res.reason = "Not Found";
                res.body = "404 Not Found\n";
            }
        }
    }
    else if (req.method == "POST") {
        if (req.target == "/echo") {
            res.body = "You POSTed:\n" + req.body + "\n";
        } else {
            res.status_code = 404;
            res.reason = "Not Found";
            res.body = "Unknown POST path\n";
        }
    }
    else {
        res.status_code = 405;
        res.reason = "Method Not Allowed";
        res.body = "Only GET and POST supported\n";
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

bool net_server::serve_static(const std::string& doc_root,
                  const HttpRequest& req,
                  HttpResponse& res)
{
    // Only handle GET for now
    if (req.method != "GET") {
        std::cerr << "No GET method\n";
        return false;
    }

    std::string path = req.target;

    // Normalize "/" -> "/index.html"
    if (path == "/") {
        path = "/index.html";
    }

    // Simple security: disallow ".."
    if (path.find("..") != std::string::npos) {
        res.status_code = 400;
        res.reason = "Bad Request";
        res.body = "Invalid path\n";
        return true; // we *did* handle it (with an error)
    }

    // Strip leading '/'
    if (!path.empty() && path[0] == '/') {
        path.erase(0, 1);
    }

    // Build filesystem path: doc_root + "/" + path
    std::string fs_path = doc_root;
    if (!fs_path.empty() && fs_path.back() != '/' && fs_path.back() != '\\') {
        fs_path += '/';
    }
    fs_path += path;

    // Try to open the file
    std::ifstream file(fs_path, std::ios::binary);
    if (!file) {
        std::cerr << "File not found: " << fs_path << '\n';
        // File not found
        return false;  // let caller decide 404 or other behavior
    }

    // Read file to body
    std::string body;
    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();
    if (size > 0) {
        body.resize(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(&body[0], size);
    }

    // Guess Content-Type from extension
    std::string content_type = "application/octet-stream";
    auto dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot + 1);
        if (ext == "html" || ext == "htm") content_type = "text/html";
        else if (ext == "txt")             content_type = "text/plain";
        else if (ext == "css")             content_type = "text/css";
        else if (ext == "js")              content_type = "application/javascript";
        else if (ext == "json")            content_type = "application/json";
        // you can add more later
    }

    res.status_code = 200;
    res.reason = "OK";
    res.body = std::move(body);
    res.set_header("Content-Type", content_type);

    return true;
}
