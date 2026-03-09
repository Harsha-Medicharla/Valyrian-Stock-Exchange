#!/usr/bin/env sh
set -eu
docker compose down
docker compose up
# docker compose up --build
