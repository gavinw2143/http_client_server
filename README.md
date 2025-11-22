# simple_http

A small C++ HTTP/1.1 server + client with:
- cross-platform socket wrapper
- HTTP parsing + routing
- static file serving
- multithreaded client request handling
- TLS support (OpenSSL)
- GoogleTest-based test suite

## Build

```bash
git clone https://github.com/gavinw2143/simple_http.git
cd simple_http
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure   # run tests
```

## TLS

The server runs with TLS enabled, using a self-signed certificate.

1. Generate a local cert:

```bash
mkdir -p certs
cd certs
openssl genrsa -out server.key 2048
openssl req -new -x509 -key server.key -out server.crt -days 365 \
  -subj "/CN=localhost"
```

2. Start the server:

```bash
./build/http_server_main
```

3. Test with curl:

```bash
curl -k https://127.0.0.1:8443/index.html
```

Note: certs/ is gitignored on purpose; do not commit private keys.

## Static files (www/)

Static files are served from the www/ directory at the project root.

Example:

`www/index.html` → `GET /index.html`

`www/hello.txt` → `GET /hello.txt`

You can edit or add files in `www/` to experiment with static serving.
