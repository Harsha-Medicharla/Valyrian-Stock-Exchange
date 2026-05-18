FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

ARG CACHE_BUST=5

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    ninja-build \
    python3 \
    git \
    libpq-dev \
    postgresql-client \
    libssl-dev \
    libhiredis-dev \
    uuid-dev \
    libjsoncpp-dev \
    pkg-config \
    zlib1g-dev \
    libbrotli-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target trade_server_app api_server_app -j$(nproc)

RUN mkdir -p /app/data

CMD ["./build/trade_server/trade_server_app"]
