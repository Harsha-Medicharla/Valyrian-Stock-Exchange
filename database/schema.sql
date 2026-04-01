-- Valyrian Stock Exchange: Full Normalized Schema (PostgreSQL)
-- Matches Settlement Module Specification

-- =========================================================
-- Cleanup (Optional, use with care)
-- =========================================================
DROP TABLE IF EXISTS financials CASCADE;
DROP TABLE IF EXISTS fundamentals CASCADE;
DROP TABLE IF EXISTS overview CASCADE;
DROP TABLE IF EXISTS order_history CASCADE;
DROP TABLE IF EXISTS user_holdings CASCADE;
DROP TABLE IF EXISTS user_funds CASCADE;
DROP TABLE IF EXISTS symbols CASCADE;
DROP TABLE IF EXISTS users CASCADE;
DROP TYPE IF EXISTS kyc_status_enum CASCADE;
DROP TYPE IF EXISTS account_type_enum CASCADE;

-- =========================================================
-- ENUM Types
-- =========================================================
DO $$ BEGIN
    CREATE TYPE kyc_status_enum AS ENUM ('PENDING', 'VERIFIED', 'REJECTED');
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

DO $$ BEGIN
    CREATE TYPE account_type_enum AS ENUM ('RETAIL', 'HFT', 'INSTITUTIONAL');
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

-- =========================================================
-- 1. Users
-- =========================================================
CREATE TABLE IF NOT EXISTS users (
    user_id         BIGSERIAL PRIMARY KEY,
    name            VARCHAR(255) NOT NULL,
    email           VARCHAR(255) UNIQUE NOT NULL,
    phone           VARCHAR(20),
    pan_number      VARCHAR(20),
    account_number  VARCHAR(50),
    demat_account   VARCHAR(50),
    kyc_status      kyc_status_enum NOT NULL DEFAULT 'PENDING',
    account_type    account_type_enum NOT NULL DEFAULT 'RETAIL',
    password_hash   VARCHAR(255) NOT NULL,
    is_active       BOOLEAN DEFAULT TRUE,
    created_at      TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- =========================================================
-- 2. User Funds
-- =========================================================
CREATE TABLE IF NOT EXISTS user_funds (
    user_id         BIGINT PRIMARY KEY REFERENCES users(user_id) ON DELETE CASCADE,
    cash_balance    BIGINT NOT NULL DEFAULT 0,
    blocked_funds   BIGINT NOT NULL DEFAULT 0,
    last_synced_at  TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- =========================================================
-- 3. Symbols
-- =========================================================
CREATE TABLE IF NOT EXISTS symbols (
    symbol_id       BIGSERIAL PRIMARY KEY,
    symbol          VARCHAR(10) UNIQUE NOT NULL,
    company_name    VARCHAR(255) NOT NULL,
    exchange        VARCHAR(10) NOT NULL DEFAULT 'NSE'
);

-- =========================================================
-- 4. User Holdings
-- =========================================================
CREATE TABLE IF NOT EXISTS user_holdings (
    user_id         BIGINT REFERENCES users(user_id) ON DELETE CASCADE,
    symbol_id       BIGINT REFERENCES symbols(symbol_id) ON DELETE RESTRICT,
    quantity        BIGINT NOT NULL DEFAULT 0,
    blocked_qty     BIGINT NOT NULL DEFAULT 0,
    avg_buy_price   BIGINT NOT NULL DEFAULT 0,
    invested_value  BIGINT NOT NULL DEFAULT 0,
    first_buy_date  DATE,
    last_synced_at  TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, symbol_id)
);

-- =========================================================
-- 5. Order History
-- =========================================================
CREATE TABLE IF NOT EXISTS order_history (
    id              BIGSERIAL PRIMARY KEY,
    user_id         BIGINT REFERENCES users(user_id) ON DELETE SET NULL,
    symbol_id       BIGINT REFERENCES symbols(symbol_id) ON DELETE RESTRICT,
    side            VARCHAR(10) NOT NULL,
    type            VARCHAR(10) NOT NULL,
    status          VARCHAR(20) NOT NULL,
    price           BIGINT NOT NULL,
    quantity        BIGINT NOT NULL,
    created_at      TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_order_history_user_id ON order_history(user_id);
CREATE INDEX IF NOT EXISTS idx_order_history_symbol_id ON order_history(symbol_id);

-- =========================================================
-- 6. Overview (OHLCV + Circuit Limits)
-- =========================================================
CREATE TABLE IF NOT EXISTS overview (
    symbol_id       BIGINT REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    date            DATE NOT NULL,
    open            BIGINT,
    high            BIGINT,
    low             BIGINT,
    close           BIGINT,
    volume          BIGINT DEFAULT 0,
    upper_circuit   BIGINT,
    lower_circuit   BIGINT,
    PRIMARY KEY (symbol_id, date)
);

-- =========================================================
-- 7. Fundamentals
-- =========================================================
CREATE TABLE IF NOT EXISTS fundamentals (
    symbol_id       BIGINT REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    date            DATE NOT NULL,
    market_cap      BIGINT,
    pe_ratio        DOUBLE PRECISION,
    pb_ratio        DOUBLE PRECISION,
    industry_pe     DOUBLE PRECISION,
    roe             DOUBLE PRECISION,
    eps_ttm         DOUBLE PRECISION,
    dividend_yield  DOUBLE PRECISION,
    book_value      DOUBLE PRECISION,
    debt_to_equity  DOUBLE PRECISION,
    face_value      DOUBLE PRECISION,
    PRIMARY KEY (symbol_id, date)
);

-- =========================================================
-- 8. Financials
-- =========================================================
CREATE TABLE IF NOT EXISTS financials (
    symbol_id       BIGINT REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    year            BIGINT NOT NULL,
    revenue         BIGINT,
    profit          BIGINT,
    net_worth       BIGINT,
    PRIMARY KEY (symbol_id, year)
);
