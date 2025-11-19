#include "http_message.hpp"

static bool parse_headers_and_body(const std::string& raw,
                                   std::size_t start_pos,
                                   std::vector<HttpHeader>& out_headers,
                                   std::string& out_body,
                                   std::string* error_message) {
    auto set_error = [&](std::string msg) {
        if (error_message) *error_message = std::move(msg);
    };

    std::vector<std::string> header_lines;
    std::string current;
    bool saw_blank_line = false;

    while (start_pos + 1 < raw.size()) {
        char c  = raw[start_pos];
        char c2 = raw[start_pos + 1];

        if (c == '\r' && c2 == '\n') {
            if (current.empty()) {
                // Empty line: end of headers
                start_pos += 2;
                saw_blank_line = true;
                break;
            } else {
                header_lines.push_back(current);
                current.clear();
                start_pos += 2;
                continue;
            }
        }

        current.push_back(c);
        ++start_pos;
    }

    if (!saw_blank_line) {
        set_error("Reached end of input while parsing headers (no blank line)");
        return false;
    }

    for (const auto& line : header_lines) {
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string name  = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        std::size_t first_non_space = value.find_first_not_of(" \t");
        if (first_non_space != std::string::npos) {
            value.erase(0, first_non_space);
        } else {
            value.clear();
        }

        out_headers.push_back(HttpHeader{std::move(name), std::move(value)});
    }

    out_body = raw.substr(start_pos);

    return true;
}

void HttpResponse::set_header(std::string name, std::string value) {
    // naive: overwrite if exists, else push_back
    for (auto& h : headers) {
        if (h.name == name) {
            h.value = std::move(value);
            return;
        }
    }
    headers.push_back(HttpHeader{std::move(name), std::move(value)});
}

std::optional<std::string> HttpRequest::header_value(std::string_view name) const {
    for (const auto& h : headers) {
        if (h.name == name) { // we can improve this later (case-insensitive)
            return h.value;
        }
    }
    return std::nullopt;
}

bool parse_http_request(const std::string &raw, HttpRequest &out, std::string* error_message) {
    auto set_error = [&](std::string msg) {
        if (error_message) *error_message = std::move(msg);
    };

    std::size_t line_end = raw.find("\r\n");

    if (line_end == std::string::npos) {
        set_error("No CRLF in request (missing request line terminator)");
        return false;
    }

    std::string request_line = raw.substr(0, line_end);

    std::size_t first_space = request_line.find(' ');
    if (first_space == std::string::npos) {
        set_error("Malformed request line (missing first space)");
        return false;
    }

    std::size_t second_space = request_line.find(' ', first_space + 1);
    if (second_space == std::string::npos) {
        set_error("Malformed request line (missing second space)");
        return false;
    }

    out.method  = request_line.substr(0, first_space);
    out.target    = request_line.substr(first_space + 1, second_space - first_space - 1);
    out.version = request_line.substr(second_space + 1);

    if (!parse_headers_and_body(raw, line_end + 2, out.headers, out.body, error_message)) {
      return false;
    }

    return true;
}

bool parse_http_response(const std::string& raw,
                         HttpResponse& out,
                         std::string* error_message)
{
    auto set_error = [&](std::string msg) {
        if (error_message) *error_message = std::move(msg);
    };

    std::size_t line_end = raw.find("\r\n");
    if (line_end == std::string::npos) {
        set_error("No CRLF in response (missing status line terminator)");
        return false;
    }

    std::string status_line = raw.substr(0, line_end);

    // 2) Split: version status_code reason_phrase
    std::size_t first_space  = status_line.find(' ');
    if (first_space == std::string::npos) {
        set_error("Malformed status line (missing first space)");
        return false;
    }

    std::size_t second_space = status_line.find(' ', first_space + 1);
    if (second_space == std::string::npos) {
        set_error("Malformed status line (missing second space)");
        return false;
    }

    std::string version_str = status_line.substr(0, first_space);
    std::string code_str    = status_line.substr(first_space + 1,
                                second_space - (first_space + 1));
    std::string reason_str  = status_line.substr(second_space + 1);

    // Parse status_code
    try {
        out.status_code = std::stoi(code_str);
    } catch (...) {
        set_error("Invalid status code in status line");
        return false;
    }

    out.reason = std::move(reason_str);

    if (!parse_headers_and_body(raw, line_end + 2, out.headers, out.body, error_message)) {
      return false;
    }

    return true;
}

std::string serialize_http_response(const HttpResponse& res) {
    std::string out;

    out += "HTTP/1.1 ";
    out += std::to_string(res.status_code);
    out += " ";
    out += res.reason;
    out += "\r\n";

    for (const auto& h : res.headers) {
        out += h.name;
        out += ": ";
        out += h.value;
        out += "\r\n";
    }

    out += "\r\n";

    out += res.body;

    return out;
}
