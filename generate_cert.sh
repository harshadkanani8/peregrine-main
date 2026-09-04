#!/usr/bin/env bash
# Generates a self-signed certificate + private key for local HTTPS testing.
# NOT for production use — browsers/curl will flag it as untrusted since
# no real Certificate Authority signed it (that's expected for localhost dev).
set -e
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout key.pem -out cert.pem -days 365 \
  -subj "/C=US/ST=Dev/L=Local/O=Demo/CN=localhost"
echo "Generated cert.pem and key.pem"
