CREATE TABLE IF NOT EXISTS users (
    user_id       BIGSERIAL PRIMARY KEY,
    email         VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    name          VARCHAR(255),
    is_active     BOOLEAN NOT NULL DEFAULT true
);

CREATE TABLE IF NOT EXISTS balances (
    user_id   BIGINT PRIMARY KEY REFERENCES users(user_id),
    available BIGINT NOT NULL DEFAULT 0,
    blocked   BIGINT NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS symbols (
    symbol_id    SERIAL PRIMARY KEY,
    ticker       VARCHAR(20) UNIQUE NOT NULL,
    company_name VARCHAR(255),
    tick_size    BIGINT NOT NULL DEFAULT 1,
    lot_size     INT    NOT NULL DEFAULT 1,
    is_active    BOOLEAN NOT NULL DEFAULT true
);

CREATE TABLE IF NOT EXISTS holdings (
    user_id       BIGINT NOT NULL REFERENCES users(user_id),
    symbol_id     INT    NOT NULL REFERENCES symbols(symbol_id),
    available_qty INT    NOT NULL DEFAULT 0,
    blocked_qty   INT    NOT NULL DEFAULT 0,
    PRIMARY KEY (user_id, symbol_id)
);

CREATE TABLE IF NOT EXISTS orders (
    order_id   BIGINT PRIMARY KEY,
    user_id    BIGINT NOT NULL REFERENCES users(user_id),
    symbol_id  INT    NOT NULL REFERENCES symbols(symbol_id),
    price      BIGINT NOT NULL,
    qty        INT    NOT NULL,
    filled_qty INT    NOT NULL DEFAULT 0,
    side       SMALLINT NOT NULL,
    type       SMALLINT NOT NULL,
    status     SMALLINT NOT NULL DEFAULT 0,
    timestamp  BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS trades (
    trade_id       BIGSERIAL PRIMARY KEY,
    symbol_id      INT    NOT NULL REFERENCES symbols(symbol_id),
    buyer_order_id  BIGINT NOT NULL REFERENCES orders(order_id),
    seller_order_id BIGINT NOT NULL REFERENCES orders(order_id),
    buyer_user_id  BIGINT NOT NULL,
    seller_user_id BIGINT NOT NULL,
    price          BIGINT NOT NULL,
    qty            INT    NOT NULL,
    timestamp      BIGINT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_orders_user   ON orders(user_id);
CREATE INDEX IF NOT EXISTS idx_orders_symbol ON orders(symbol_id, status);
CREATE INDEX IF NOT EXISTS idx_trades_symbol ON trades(symbol_id, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_trades_buyer  ON trades(buyer_user_id);
CREATE INDEX IF NOT EXISTS idx_trades_seller ON trades(seller_user_id);
