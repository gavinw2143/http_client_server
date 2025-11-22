# TLS certificates

To generate a self-signed cert for local testing:

```bash
mkdir -p certs
cd certs

openssl genrsa -out server.key 2048
openssl req -new -x509 -key server.key -out server.crt -days 365 \
  -subj "/CN=localhost"
