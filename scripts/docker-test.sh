#!/bin/bash

set -e

IMAGE_NAME="password-vault:latest"
CONTAINER_NAME="password-vault-test"

echo "Testing Docker image: $IMAGE_NAME"

echo "Cleaning up any existing test container..."
docker rm -f $CONTAINER_NAME 2>/dev/null || true

echo "Starting container..."
docker run -d \
  --name $CONTAINER_NAME \
  -p 8443:8443 \
  $IMAGE_NAME

echo "Waiting for container to be healthy..."
sleep 5

echo "Checking if container is running..."
if ! docker ps | grep -q $CONTAINER_NAME; then
    echo "ERROR: Container failed to start"
    docker logs $CONTAINER_NAME
    docker rm -f $CONTAINER_NAME
    exit 1
fi

echo "Testing HTTPS endpoint..."
HTTP_CODE=$(curl -k -s -o /dev/null -w "%{http_code}" https://localhost:8443/ || echo "000")

if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "404" ]; then
    echo "SUCCESS: Server is responding (HTTP $HTTP_CODE)"
else
    echo "ERROR: Server not responding correctly (HTTP $HTTP_CODE)"
    docker logs $CONTAINER_NAME
    docker rm -f $CONTAINER_NAME
    exit 1
fi

echo "Checking container logs..."
docker logs $CONTAINER_NAME | head -20

echo "Stopping and removing test container..."
docker stop $CONTAINER_NAME
docker rm $CONTAINER_NAME

echo "All Docker tests passed!"