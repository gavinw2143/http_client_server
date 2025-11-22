#include "net_tls.hpp"
#include <stdexcept>
#include <string>

namespace net {

TlsContext::TlsContext(const char* cert_file, const char* key_file) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        throw std::runtime_error("SSL_CTX_new failed");
    }

    if (SSL_CTX_use_certificate_file(ctx_, cert_file, SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("SSL_CTX_use_certificate_file failed");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx_, key_file, SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("SSL_CTX_use_PrivateKey_file failed");
    }
    if (!SSL_CTX_check_private_key(ctx_)) {
        throw std::runtime_error("SSL_CTX_check_private_key failed");
    }
}

TlsContext::~TlsContext() {
    if (ctx_) {
        SSL_CTX_free(ctx_);
        ctx_ = nullptr;
    }
    EVP_cleanup();
}

TlsSocket::TlsSocket(TlsContext& ctx, net::Socket&& underlying)
    : socket_(std::move(underlying))
{
    ssl_ = SSL_new(ctx.get());
    if (!ssl_) {
        throw std::runtime_error("SSL_new failed");
    }

    SSL_set_fd(ssl_, socket_.native_handle());

    int ret = SSL_accept(ssl_);
    if (ret <= 0) {
        int err = SSL_get_error(ssl_, ret);
        SSL_free(ssl_);
        ssl_ = nullptr;
        throw std::runtime_error("SSL_accept failed with error " + std::to_string(err));
    }
}

TlsSocket::TlsSocket(TlsSocket&& other) noexcept
    : socket_(std::move(other.socket_))
    , ssl_(other.ssl_)
{
    other.ssl_ = nullptr;
}

TlsSocket& TlsSocket::operator=(TlsSocket&& other) noexcept {
    if (this != &other) {
        if (ssl_) {
            SSL_shutdown(ssl_);
            SSL_free(ssl_);
        }

        socket_ = std::move(other.socket_);
        ssl_    = other.ssl_;
        other.ssl_ = nullptr;
    }
    return *this;
}

TlsSocket::~TlsSocket() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
}

std::size_t TlsSocket::recv(void* buf, std::size_t len) {
    int ret = SSL_read(ssl_, buf, static_cast<int>(len));
    if (ret <= 0) {
        int err = SSL_get_error(ssl_, ret);
        if (err == SSL_ERROR_ZERO_RETURN) {
            return 0; // clean shutdown
        }
        throw std::runtime_error("SSL_read failed with error " + std::to_string(err));
    }
    return static_cast<std::size_t>(ret);
}

void TlsSocket::send_all(const void* buf, std::size_t len) {
    const unsigned char* p = static_cast<const unsigned char*>(buf);
    std::size_t sent = 0;
    while (sent < len) {
        int ret = SSL_write(ssl_, p + sent, static_cast<int>(len - sent));
        if (ret <= 0) {
            int err = SSL_get_error(ssl_, ret);
            throw std::runtime_error("SSL_write failed with error " + std::to_string(err));
        }
        sent += static_cast<std::size_t>(ret);
    }
}

}
