#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include "http_server.hpp"
#include <csignal>


namespace net_server {
namespace {    
    std::atomic_bool g_running{true};

    extern "C" void handle_sigint(int) {
        g_running.store(false, std::memory_order_relaxed);
    }

    bool read_http_request(net::Socket& client,
                           HttpRequest& out_req,
                           std::string& error)
    {
        std::string raw;
        char buf[4096];
        std::size_t header_end = std::string::npos;

        // read until we see \r\n\r\n or EOF
        for (;;) {
            std::size_t n = client.recv(buf, sizeof(buf));
            if (n == 0) {
                break;
            }
            raw.append(buf, n);

            auto pos = raw.find("\r\n\r\n");
            if (pos != std::string::npos) {
                header_end = pos + 4;
                break;
            }
        }

        if (header_end == std::string::npos) {
            if (raw.empty()) {
                // client connected and closed without sending anything
                error = "Empty request";
                return false;
            }
            error = "No header terminator";
            return false;
        }

        // parse just the header part
        std::string head = raw.substr(0, header_end);

        if (!parse_http_request(head, out_req, &error)) {
            return false;
        }

        // handle Content-Length body
        std::size_t content_length = 0;
        if (auto len_str = out_req.header_value("Content-Length")) {
            try {
                content_length = static_cast<std::size_t>(std::stoul(*len_str));
            } catch (...) {
                error = "Invalid Content-Length";
                return false;
            }
        }

        // seed body with whatever we already read after headers
        out_req.body = raw.substr(header_end);
        std::size_t received = out_req.body.size();

        while (received < content_length) {
            std::size_t to_read = content_length - received;
            if (to_read > sizeof(buf)) {
                to_read = sizeof(buf);
            }

            std::size_t n = client.recv(buf, to_read);
            if (n == 0) {
                error = "Client closed during body";
                return false;
            }

            out_req.body.append(buf, n);
            received += n;
        }

        return true;
    }

    void route_request(const HttpRequest& req, HttpResponse& res) {
        // Default response
        res.status_code = 200;
        res.reason      = "OK";
        res.body.clear();
        res.headers.clear();

        if (req.method == "GET") {

            if (req.target == "/hello") {
                res.body = "Hello endpoint\n";
            }
            else if (req.target == "/slow") {
                // optional: keep your test slow endpoint
                std::this_thread::sleep_for(std::chrono::seconds(3));
                res.body = "Slow endpoint\n";
            }
            else if (net_server::serve_static("./www", req, res)) {
                // static handler already filled res (200 or 400)
            }
            else {
                res.status_code = 404;
                res.reason      = "Not Found";
                res.body        = "404 Not Found\n";
            }
        }
        else if (req.method == "POST") {

            if (req.target == "/echo") {
                res.body = "You POSTed:\n" + req.body + "\n";
            } else {
                res.status_code = 404;
                res.reason      = "Not Found";
                res.body        = "Unknown POST path\n";
            }
        }
        else {
            res.status_code = 405;
            res.reason      = "Method Not Allowed";
            res.body        = "Only GET and POST supported\n";
        }

        // Note: Content-Type gets set later (or inside serve_static).
    }

    void add_common_headers(HttpResponse& res) {
        // Only add Content-Type if not already set (static files might have set it)
        bool has_ct = false;
        for (const auto& h : res.headers) {
            if (h.name == "Content-Type") {
                has_ct = true;
                break;
            }
        }
        if (!has_ct) {
            res.set_header("Content-Type", "text/plain");
        }

        res.set_header("Content-Length", std::to_string(res.body.size()));
        res.set_header("Connection", "close");
    }

    std::mutex g_log_mutex;

    void log_line(std::string_view text) {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cout << text << '\n';
    }

    void log_http_access(const HttpRequest* req,
                         const HttpResponse& res,
                         std::string_view note = {})
    {
        std::ostringstream oss;
        if (req) {
            oss << "[http] " << req->method << " " << req->target;
        } else {
            oss << "[http] (no-request)";
        }

        oss << " -> " << res.status_code << " " << res.reason;
        if (!note.empty()) {
            oss << " (" << note << ")";
        }

        log_line(oss.str());
    }

    void send_http_response_and_log(net::Socket& client,
                                    const HttpRequest* req,
                                    const HttpResponse& res,
                                    std::string_view note = {})
    {
        std::string response_str = serialize_http_response(res);
        client.send_all(response_str.data(), response_str.size());
        log_http_access(req, res, note);
    }
}
}

void net_server::run_http_server(uint16_t port) {
    net::Socket listener = net::Socket::tcp_v4();
    listener.bind_v4_any(port);
    listener.listen();

    std::signal(SIGINT, handle_sigint);

    std::cout << "Listening on port " << port << "...\n";

    while (g_running.load(std::memory_order_relaxed)) {
        std::cout << "Waiting for connection...\n";

        net::Socket client;
        try {
            client = listener.accept();
        } catch (const std::exception& ex) {
            if (!g_running.load(std::memory_order_relaxed)) {
                std::cout << "[http] accept interrupted, shutting down\n";
                break;
            }
            std::cerr << "[http] accept error: " << ex.what() << "\n";
            continue; 
        }

        std::cout << "Accepted connection!\n";

        // Spawn a thread to handle this client
        std::thread t(
            [](net::Socket c) {
                try {
                    net_server::handle_client(std::move(c));
                } catch (const std::exception& ex) {
                    std::cerr << "[http] Client handler threw: " << ex.what() << "\n";
                }
            },
            std::move(client)   // move the socket into the thread
        );

        t.detach(); // we don’t join; thread cleans up itself when done
    }
}

void net_server::handle_client(net::Socket client) {
    HttpRequest  req;
    HttpResponse res;
    std::string  error;

    if (!read_http_request(client, req, error)) {
        res.status_code = 400;
        res.reason      = "Bad Request";
        res.body        = "Bad Request\n";

        add_common_headers(res);
        send_http_response_and_log(client, nullptr, res, error);
        return;
    }

    route_request(req, res);

    add_common_headers(res);
    send_http_response_and_log(client, &req, res);
}


bool net_server::serve_static(const std::string& doc_root,
                  const HttpRequest& req,
                  HttpResponse& res)
{
    if (req.method != "GET") {
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
        res.set_header("Content-Type", "text/plain");
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
        return false;  // let caller decide 404
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
    }

    res.status_code = 200;
    res.reason = "OK";
    res.body = std::move(body);
    res.set_header("Content-Type", content_type);

    return true;
}
