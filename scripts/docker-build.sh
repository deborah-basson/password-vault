#!/bin/bash

set -e

echo "Building Password Vault application..."
make clean
make

echo "Generating SSL certificates if needed..."
if [ ! -f cert.pem ] || [ ! -f key.pem ]; then
    make ssl
fi

echo "Building Docker image..."
docker build -t password-vault:latest .

echo "Docker image built successfully!"
docker images | grep password-vault