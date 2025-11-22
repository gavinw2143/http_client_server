#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>

struct HttpHeader {
    std::string name;
    std::string value;
};

struct HttpRequest {
    std::string method;    // "GET"
    std::string target;    // "/hello"
    std::string version;   // "HTTP/1.1"
    std::vector<HttpHeader> headers;
    std::string body;

    // Convenience lookup (case-insensitive would be nicer later)
    std::optional<std::string> header_value(std::string_view name) const;
};

struct HttpResponse {
    int status_code = 200;               // 200, 404, etc.
    std::string reason = "OK";           // "OK", "Not Found", etc.
    std::vector<HttpHeader> headers;
    std::string body;

    void set_header(std::string name, std::string value);
};

// Parse a full HTTP request from a raw string (headers + optional body).
// Returns true on success, false on parse error.
bool parse_http_request(const std::string& raw,
                        HttpRequest& out,
                        std::string* error_message = nullptr);

bool parse_http_response(const std::string& raw,
                         HttpResponse& out,
                         std::string* error_message = nullptr);

// Serialize an HttpResponse into a wire-format HTTP response.
std::string serialize_http_response(const HttpResponse& res);

