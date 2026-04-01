-- Valyrian Stock Exchange: Core Normalized Schema (PostgreSQL)

CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS symbols (
    id BIGSERIAL PRIMARY KEY,
    ticker VARCHAR(10) UNIQUE NOT NULL,
    company_name VARCHAR(255) NOT NULL,
    is_tradable BOOLEAN DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS user_funds (
    user_id BIGINT PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    available_cash DECIMAL(19, 4) NOT NULL DEFAULT 0.0000,
    blocked_cash DECIMAL(19, 4) NOT NULL DEFAULT 0.0000
);

CREATE TABLE IF NOT EXISTS user_holdings (
    user_id BIGINT REFERENCES users(id) ON DELETE CASCADE,
    symbol_id BIGINT REFERENCES symbols(id) ON DELETE RESTRICT,
    available_qty BIGINT NOT NULL DEFAULT 0,
    blocked_qty BIGINT NOT NULL DEFAULT 0,
    PRIMARY KEY (user_id, symbol_id)
);

CREATE TABLE IF NOT EXISTS order_history (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    symbol_id BIGINT REFERENCES symbols(id) ON DELETE RESTRICT,
    side VARCHAR(10) NOT NULL, -- 'BUY' or 'SELL'
    type VARCHAR(10) NOT NULL, -- 'LIMIT' or 'MARKET'
    status VARCHAR(20) NOT NULL, -- 'FILLED', 'PARTIAL', 'CANCELLED'
    price DECIMAL(19, 4) NOT NULL,
    quantity BIGINT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Indexes for fast API reads
CREATE INDEX IF NOT EXISTS idx_order_history_user_id ON order_history(user_id);
CREATE INDEX IF NOT EXISTS idx_order_history_symbol_id ON order_history(symbol_id);
