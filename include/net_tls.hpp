#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>
#include "socket.hpp"

namespace net {

class TlsContext {
public:
    TlsContext(const char* cert_file, const char* key_file);
    ~TlsContext();

    SSL_CTX* get() const { return ctx_; }

private:
    SSL_CTX* ctx_ = nullptr;
};

class TlsSocket {
public:
    TlsSocket(TlsContext& ctx, net::Socket&& underlying);
    ~TlsSocket();

    // no copy
    TlsSocket(const TlsSocket&) = delete;
    TlsSocket& operator=(const TlsSocket&) = delete;

    // move
    TlsSocket(TlsSocket&& other) noexcept;
    TlsSocket& operator=(TlsSocket&& other) noexcept;

    std::size_t recv(void* buf, std::size_t len);
    void send_all(const void* buf, std::size_t len);

private:
    net::Socket socket_;
    SSL* ssl_ = nullptr;
};

} // namespace net
