#!/usr/bin/env bash

podman buildx build -f Dockerfile.rocky9 --output ./artifacts/rocky9 ../../
