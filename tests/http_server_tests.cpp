#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "../http_server.hpp"

namespace fs = std::filesystem;

// / -> index.html
TEST(ServeStaticTest, ServesIndexForRootPath) {
    fs::path doc_root = "test_www_root";
    fs::create_directories(doc_root);

    std::string index_content = "<h1>Index</h1>";
    {
        std::ofstream ofs(doc_root / "index.html", std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << index_content;
    }

    HttpRequest req;
    req.method = "GET";
    req.target = "/";

    HttpResponse res;

    bool handled = net_server::serve_static(doc_root.string(), req, res);

    EXPECT_TRUE(handled);
    EXPECT_EQ(res.status_code, 200);
    EXPECT_EQ(res.reason, "OK");
    EXPECT_EQ(res.body, index_content);

    fs::remove_all(doc_root);
}

// /file.txt -> file.txt
TEST(ServeStaticTest, ServesArbitraryFileUnderDocRoot) {
    fs::path doc_root = "test_www_root2";
    fs::create_directories(doc_root);

    std::string file_content = "Hello from file.txt\n";
    {
        std::ofstream ofs(doc_root / "file.txt", std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << file_content;
    }

    HttpRequest req;
    req.method = "GET";
    req.target = "/file.txt";

    HttpResponse res;

    bool handled = net_server::serve_static(doc_root.string(), req, res);

    EXPECT_TRUE(handled);
    EXPECT_EQ(res.status_code, 200);
    EXPECT_EQ(res.body, file_content);

    fs::remove_all(doc_root);
}

TEST(ServeStaticTest, RejectsPathTraversalOutsideDocRoot) {
    namespace fs = std::filesystem;

    fs::path doc_root = "test_www_root3";
    fs::create_directories(doc_root);

    HttpRequest req;
    req.method = "GET";
    req.target = "/../secret.txt";  // attempt to escape

    HttpResponse res;

    bool handled = net_server::serve_static(doc_root.string(), req, res);

    EXPECT_TRUE(handled);
    EXPECT_EQ(res.status_code, 400);
    EXPECT_EQ(res.reason, "Bad Request");
    EXPECT_FALSE(res.body.empty());

    fs::remove_all(doc_root);
}

TEST(ServeStaticTest, ReturnsFalseForNonGetMethods) {
    namespace fs = std::filesystem;

    fs::path doc_root = "test_www_root_nonget";
    fs::create_directories(doc_root);

    // even if the file exists, static handler shouldn't handle non-GET
    {
        std::ofstream ofs(doc_root / "index.html", std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << "Should not be served for POST";
    }

    HttpRequest req;
    req.method = "POST";
    req.target = "/";

    HttpResponse res;
    res.status_code = 0;  // sentinel

    bool handled = net_server::serve_static(doc_root.string(), req, res);

    // For non-GET, we want the static handler to say "I didn't handle this"
    EXPECT_FALSE(handled);

    // Optionally: confirm we didn't touch the response
    EXPECT_EQ(res.status_code, 0);

    fs::remove_all(doc_root);
}

// HTML should be text/html
TEST(ServeStaticTest, SetsContentTypeForHtml) {
    fs::path doc_root = "test_www_root_ct_html";
    fs::create_directories(doc_root);

    std::string content = "<h1>HTML</h1>";
    {
        std::ofstream ofs(doc_root / "index.html", std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << content;
    }

    HttpRequest req;
    req.method = "GET";
    req.target = "/";   // normalized to /index.html

    HttpResponse res;

    bool handled = net_server::serve_static(doc_root.string(), req, res);

    ASSERT_TRUE(handled);
    EXPECT_EQ(res.status_code, 200);
    EXPECT_EQ(res.body, content);

    // Find Content-Type header
    std::string ct;
    for (const auto& h : res.headers) {
        if (h.name == "Content-Type") {
            ct = h.value;
            break;
        }
    }
    EXPECT_EQ(ct, "text/html");

    fs::remove_all(doc_root);
}

// .txt should be text/plain
TEST(ServeStaticTest, SetsContentTypeForTxt) {
    fs::path doc_root = "test_www_root_ct_txt";
    fs::create_directories(doc_root);

    std::string content = "Plain text\n";
    {
        std::ofstream ofs(doc_root / "file.txt", std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs << content;
    }

    HttpRequest req;
    req.method = "GET";
    req.target = "/file.txt";

    HttpResponse res;

    bool handled = net_server::serve_static(doc_root.string(), req, res);

    ASSERT_TRUE(handled);
    EXPECT_EQ(res.status_code, 200);
    EXPECT_EQ(res.body, content);

    std::string ct;
    for (const auto& h : res.headers) {
        if (h.name == "Content-Type") {
            ct = h.value;
            break;
        }
    }
    EXPECT_EQ(ct, "text/plain");

    fs::remove_all(doc_root);
}
