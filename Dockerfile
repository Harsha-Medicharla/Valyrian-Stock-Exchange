FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    ninja-build \
    python3 \
    flatbuffers-compiler \
    git \
    libpq-dev \
    libssl-dev \
    libhiredis-dev \
    uuid-dev \
    libjsoncpp-dev \
    pkg-config \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Install flatc wrapper that patches the version check
COPY flatc-wrapper.sh /usr/local/bin/flatc-wrapper
RUN chmod +x /usr/local/bin/flatc-wrapper && \
    mv /usr/bin/flatc /usr/bin/flatc-original && \
    ln -s /usr/local/bin/flatc-wrapper /usr/bin/flatc

# Configure and build
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

RUN cmake --build build --target trade_server_app api_server_app -j2

CMD ["./build/trade_server/trade_server_app"]
