#include <gtest/gtest.h>

#include "../http_message.hpp"

// Basic GET request parse
TEST(HttpMessageTest, ParseSimpleGetRequest) {
    std::string raw =
        "GET /hello HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: test-agent\r\n"
        "\r\n";

    HttpRequest req;
    std::string error;

    bool ok = parse_http_request(raw, req, &error);
    ASSERT_TRUE(ok) << "parse_http_request failed: " << error;

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.target, "/hello");
    EXPECT_EQ(req.version, "HTTP/1.1");

    // Headers
    auto host = req.header_value("Host");
    ASSERT_TRUE(host.has_value());
    EXPECT_EQ(*host, "example.com");

    auto ua = req.header_value("User-Agent");
    ASSERT_TRUE(ua.has_value());
    EXPECT_EQ(*ua, "test-agent");

    // No body for this request
    EXPECT_TRUE(req.body.empty());
}

// Basic 200 OK response parse
TEST(HttpMessageTest, ParseSimpleOkResponse) {
    std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";

    HttpResponse res;
    std::string error;

    bool ok = parse_http_response(raw, res, &error);
    ASSERT_TRUE(ok) << "parse_http_response failed: " << error;

    EXPECT_EQ(res.status_code, 200);
    EXPECT_EQ(res.reason, "OK");

    // Headers
    bool found_ct = false;
    for (const auto& h : res.headers) {
        if (h.name == "Content-Type") {
            EXPECT_EQ(h.value, "text/plain");
            found_ct = true;
        }
    }
    EXPECT_TRUE(found_ct);

    EXPECT_EQ(res.body, "hello");
}

// set_header overwrite behavior
TEST(HttpMessageTest, ResponseSetHeaderOverwrites) {
    HttpResponse res;
    res.set_header("Content-Type", "text/plain");
    res.set_header("Content-Type", "text/html");

    int count = 0;
    std::string value;
    for (const auto& h : res.headers) {
        if (h.name == "Content-Type") {
            ++count;
            value = h.value;
        }
    }

    EXPECT_EQ(count, 1);
    EXPECT_EQ(value, "text/html");
}

TEST(HttpMessageTest, RejectsRequestWithNoCRLF) {
    std::string raw = "GET / HTTP/1.1";  // no \r\n at all

    HttpRequest req;
    std::string error;

    bool ok = parse_http_request(raw, req, &error);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST(HttpMessageTest, RejectsRequestWithMissingSecondSpace) {
    std::string raw =
        "GET /\r\n"   // no HTTP version
        "Host: example.com\r\n"
        "\r\n";

    HttpRequest req;
    std::string error;

    bool ok = parse_http_request(raw, req, &error);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST(HttpMessageTest, RejectsResponseWithNoCRLF) {
    std::string raw = "HTTP/1.1 200 OK"; // no \r\n

    HttpResponse res;
    std::string error;

    bool ok = parse_http_response(raw, res, &error);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST(HttpMessageTest, RejectsResponseWithNonNumericStatusCode) {
    std::string raw =
        "HTTP/1.1 ABC OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    HttpResponse res;
    std::string error;

    bool ok = parse_http_response(raw, res, &error);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST(HttpMessageTest, HeaderValueLookupIsNameExactMatch) {
    std::string raw =
        "GET / HTTP/1.1\r\n"
        "Host   : example.com\r\n"
        "\r\n";

    HttpRequest req;
    std::string error;

    bool ok = parse_http_request(raw, req, &error);
    ASSERT_TRUE(ok) << error;

    auto host = req.header_value("Host");
    EXPECT_TRUE(host.has_value());
}

TEST(HttpMessageTest, RejectsResponseWithNoBlankLineAfterHeaders) {
    std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n";
        // NOTE: no "\r\n" blank line and no body

    HttpResponse res;
    std::string error;

    bool ok = parse_http_response(raw, res, &error);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}
