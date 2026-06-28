# Valyrian Stock Exchange

Valyrian Stock Exchange is a low-latency C++ trading system that combines a WebSocket order ingress path, an in-memory matching engine, a pre-trade EMS layer, market data publishing, write-ahead logging, database persistence, and a REST API for account and market operations.

The project is structured like an exchange core rather than a demo CRUD service: orders enter through a binary WebSocket path, move through validation and routing, are matched inside per-symbol engines, and then fan out to persistence, response delivery, and market data streams.

## Highlights

- Low-latency in-memory matching path with custom pools, intrusive price levels, and Adaptive Radix Tree based indexing
- EMS pipeline for validation, routing, rate limiting, self-trade prevention, and balance/holding checks
- Per-symbol matching engines with WAL-backed recovery hooks
- Separate REST API server for auth, balances, holdings, orders, and market queries
- Redis-backed session management and cancellation signaling
- Postgres-backed persistence for users, balances, holdings, orders, trades, and symbol metadata
- Dedicated market data publisher over WebSocket
- Unit, integration, and benchmark coverage for the hot path

## Architecture

At a high level, the system looks like this:

1. Clients authenticate through the REST API and obtain a bearer token stored in Redis.
2. Clients connect to the trade WebSocket server and submit binary FlatBuffers order messages.
3. The WebSocket layer converts inbound requests into `RawOrder` events and pushes them into EMS worker queues.
4. EMS workers validate balances, holdings, rate limits, and routing decisions, then forward accepted flow to per-symbol dispatchers.
5. Each dispatcher feeds a symbol-specific `MatchingEngine`, which updates the in-memory order book and WAL.
6. Resulting trade, order, book-update, and DB events are fanned out to:
   - response threads for execution reports
   - DB writers for Postgres persistence and balance updates
   - market data publishing threads for public feeds

Core modules:

- `trade_server/`: binary WebSocket order ingress, connection management, cancel subscription, response delivery
- `ems/`: ingress workers, validation pipeline, symbol routing, queues, dispatchers, rate limiting, market state, balance cache
- `MatchingEngine/`: order book, price levels, pools, ART index, matching logic, WAL
- `market_data/`: trade/book update fanout and candle building
- `db/`: symbol bootstrap, balance bootstrap, DB writer, Postgres writer
- `api_server/`: Drogon REST API, auth, session validation, account and market endpoints
- `shared/`: event and queue primitives shared across modules
- `tests/` and `MatchingEngine/tests/`: integration tests, unit tests, and benchmarks

## Repository Layout

```text
.
├── api_server/         # Drogon REST API
├── db/                 # Postgres schema, seed data, DB writers
├── ems/                # Validation, routing, queues, dispatchers
├── market_data/        # Market data and candle publishing
├── MatchingEngine/     # Order book, matching logic, WAL, tests, benchmarks
├── shared/             # Shared events and queue types
├── tests/              # End-to-end integration tests
├── trade_server/       # WebSocket trade ingress and response path
└── latency_tests/      # Python load and latency scripts
```

## Benchmarks and Latency

The matching engine benchmark target is defined in [`MatchingEngine/tests/matching_engine_bench.cc`](/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian-Stock-Exchange/MatchingEngine/tests/matching_engine_bench.cc) and measures core hot-path operations.

### Benchmark results

| Benchmark Name | Mean Latency | P99 Tail Latency | Variance (CV) |
| --- | ---: | ---: | ---: |
| Memory Pool (Alloc/Free) | 2.40 ns | 2.60 ns | 3.31% |
| ART & Intrusive Lists | 460 ns | 461 ns | 0.14% |
| WAL Ingestion | 2,020 ns (2.0 µs) | 2,218 ns (2.2 µs) | 3.84% |
| Engine Crossing | 24,933 ns (24.9 µs) | 25,906 ns (25.9 µs) | 2.69% |
| Deep Sweep (Stress Test) | 113,043 ns (113 µs) | 116,712 ns (116.7 µs) | 3.26% |

### System-level performance figures

- Internal matching speed: `4 ns` to `45 ns` average across 10-order windows
- Wire-to-wire RTT: about `0.1 ms` (`100 µs`)
- Max throughput: about `50,000+ orders/sec` at `5,000` concurrent orders
- API throughput: about `2,700 requests/sec` using Drogon + Postgres

## Latency Test Utilities

The `latency_tests/` directory contains Python scripts for driving the system:

- [`latency_tests/live_test.py`](/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian-Stock-Exchange/latency_tests/live_test.py): multi-user trading flow smoke test
- [`latency_tests/vse_latency_tester.py`](/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian-Stock-Exchange/latency_tests/vse_latency_tester.py): REST and WebSocket load testing
- `latency_tests/VSE/`: generated or supporting FlatBuffers Python bindings

## Tech Stack

- C++20
- CMake + Ninja
- Drogon
- PostgreSQL (`libpq`)
- Redis (`hiredis`)
- uWebSockets / uSockets
- FlatBuffers
- GoogleTest
- Google Benchmark
- Abseil flat hash map
- `libart` Adaptive Radix Tree

## Running With Docker

The easiest way to bring the stack up is:

```bash
./scripts/start-dev.sh
```

This starts:

- `postgres` on `5432`
- `redis` on `6379`
- `trade_server` on `9001`
- `market_data` on `9002`
- `api_server` on `8080`

The Docker setup initializes Postgres with:

- schema from [`db/schema.sql`](/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian-Stock-Exchange/db/schema.sql)
- seed symbols from [`db/seed.sql`](/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian-Stock-Exchange/db/seed.sql)

Seeded symbols:

- `AAPL`
- `GOOG`
- `INFY`
- `RELIANCE`

## Manual Build

The project uses top-level CMake and fetches several dependencies automatically.

### Common Linux build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Key system dependencies used by the project and Docker image:

- `build-essential`
- `cmake`
- `ninja-build`
- `git`
- `python3`
- `libpq-dev`
- `postgresql-client`
- `libssl-dev`
- `libhiredis-dev`
- `uuid-dev`
- `libjsoncpp-dev`
- `libbenchmark-dev`
- `pkg-config`
- `zlib1g-dev`
- `libbrotli-dev`

### Apple Silicon note

The CMake files include `libpq` lookup support for Homebrew installs on macOS, and the Docker setup targets `linux/arm64`.

## Build Targets

Primary executable targets:

- `trade_server_app`
- `api_server_app`
- `matching_engine_test`
- `matching_engine_bench`
- `vse_trade_flow_test`

In the Docker image, the startup commands use:

- `./build/trade_server/trade_server_app`
- `./build/api_server/api_server_app`

## Runtime Configuration

Both servers can read `config.json`, but environment variables are also supported and are what the Docker setup uses.

Important environment variables:

- `VSE_PG_CONN`: Postgres connection string
- `REDIS_HOST`
- `REDIS_PORT`
- `API_PORT`
- `API_THREADS`
- `VSE_WS_PORT`
- `VSE_MDP_WS_PORT`
- `VSE_IO_THREADS`
- `VSE_WAL_DIR`
- `VSE_CORE_MAP_JSON`
- `VSE_DEV_SKIP_AUTH`: bypasses bearer-token validation in the trade WebSocket server for development

## API Surface

The REST API is served by Drogon on port `8080`.

### Auth

- `POST /v1/signup`
- `POST /v1/sessions`
- `DELETE /v1/sessions`

### Account

- `GET /v1/account`
- `PUT /v1/account`
- `POST /v1/account/deposit`
- `POST /v1/account/deposit-holdings`

### Balance and holdings

- `GET /v1/balance`
- `GET /v1/holdings`

### Orders

- `GET /v1/orders`
- `DELETE /v1/orders/{order_id}`

### Market data and reference data

- `GET /v1/symbols`
- `GET /v1/symbols/{ticker}`
- `GET /v1/symbols/{ticker}/trades`
- `GET /healthz`

Authentication for protected REST endpoints uses:

- `Authorization: Bearer <token>`
- Redis session keys of the form `session:<token>`

## Trading Interfaces

### Trade ingress WebSocket

- Port: `9001`
- Purpose: binary order entry and execution reports
- Protocol: FlatBuffers messages defined in [`trade_server/protocol/WireTypes.fbs`](/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian-Stock-Exchange/trade_server/protocol/WireTypes.fbs)

### Market data WebSocket

- Port: `9002`
- Purpose: public trade and book update publication

## Persistence Model

The database schema currently includes:

- `users`
- `balances`
- `symbols`
- `holdings`
- `orders`
- `trades`

This supports:

- user auth and session-backed account access
- fiat balance tracking
- per-symbol holdings tracking
- order lifecycle persistence
- trade history queries

## Testing

### Unit tests

Matching engine unit tests cover:

- pool allocation and reuse
- ART insert/find/erase behavior
- price level FIFO operations
- order book operations
- matching engine state transitions

Run with:

```bash
ctest --test-dir build --output-on-failure
```

Or run specific binaries from your generated build tree:

```bash
./build/matching_engine_test || ./matching_engine_test
./build/vse_trade_flow_test || ./vse_trade_flow_test
```

### Integration coverage

The integration suite exercises end-to-end flow such as:

- ingress to match to settlement
- deterministic price/time priority
- insufficient-funds rejection
- self-trade prevention behavior

## Development Notes

- The top-level build fetches many third-party dependencies from upstream repositories at configure time.
- WAL files are stored per engine using `VSE_WAL_DIR` when set.
- The trade server boots symbol metadata from Postgres before starting workers.
- Balance state is bootstrapped into the EMS cache on startup.
- Order cancellation from the REST path is relayed through Redis pub/sub.

## Quick Start Flow

1. Start the stack with `./scripts/start-dev.sh`.
2. Create a user with `POST /v1/signup`.
3. Log in with `POST /v1/sessions` and keep the bearer token.
4. Fund the account via `POST /v1/account/deposit`.
5. Optionally deposit holdings via `POST /v1/account/deposit-holdings`.
6. Connect to `ws://localhost:9001` and submit FlatBuffers order messages.
7. Subscribe to market data on `ws://localhost:9002`.

## Current State

This repository already contains the core exchange path, persistence plumbing, API surface, and benchmark harnesses needed to run local experiments and extend the platform further. It is best understood as a systems-oriented exchange prototype with real low-latency design choices, rather than a minimal sample app.
