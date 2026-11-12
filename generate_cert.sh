#!/usr/bin/env bash
# Generates a self-signed certificate + private key for local HTTPS testing.
# NOT for production use — browsers/curl will flag it as untrusted since
# no real Certificate Authority signed it (that's expected for localhost dev).
#
# Usage:
#   ./generate_cert.sh         # Generates RSA:2048 certificate (default)
#   ./generate_cert.sh ecdsa   # Generates ECDSA P-256 certificate
set -e

if [ "$1" = "ec" ] || [ "$1" = "ecdsa" ] || [ "$1" = "p256" ]; then
  openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes \
    -keyout key.pem -out cert.pem -days 365 \
    -subj "/C=US/ST=Dev/L=Local/O=Demo/CN=localhost"
  echo "Generated ECDSA (P-256) cert.pem and key.pem"
else
  openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout key.pem -out cert.pem -days 365 \
    -subj "/C=US/ST=Dev/L=Local/O=Demo/CN=localhost"
  echo "Generated RSA (2048-bit) cert.pem and key.pem"
fi
