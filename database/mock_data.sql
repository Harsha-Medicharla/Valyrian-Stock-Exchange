-- Add standard symbols if not present
INSERT INTO symbols (symbol, company_name, exchange) VALUES 
('RELIANCE', 'Reliance Industries Ltd.', 'NSE'),
('TCS', 'Tata Consultancy Services Ltd.', 'NSE'),
('HDFCBANK', 'HDFC Bank Ltd.', 'NSE'),
('INFY', 'Infosys Ltd.', 'NSE'),
('HINDUNILVR', 'Hindustan Unilever Ltd.', 'NSE')
ON CONFLICT (symbol) DO NOTHING;

-- Create an array of symbols to easily link IDs
WITH syms AS (SELECT symbol_id, symbol FROM symbols)

-- We will insert a few fake users (harsha trader was already added)
INSERT INTO users (name, email, password_hash, kyc_status, account_type) VALUES 
('Alice Smith', 'alice@investor.com', 'alicepass', 'VERIFIED', 'RETAIL'),
('Bob Jones', 'bob@hft.com', 'bobpass', 'VERIFIED', 'HFT'),
('Capital Partners', 'admin@capital.com', 'adminpass', 'VERIFIED', 'INSTITUTIONAL')
ON CONFLICT (email) DO NOTHING;

-- Give everyone funds
INSERT INTO user_funds (user_id, cash_balance, blocked_funds)
SELECT user_id, 
       floor(random() * 500000000 + 10000000)::bigint, -- random balance between 10k to 500k rupees
       0 
FROM users 
WHERE user_id NOT IN (SELECT user_id FROM user_funds);

-- Give Harsha (trader@valyrian.com) some mock holdings
INSERT INTO user_holdings (user_id, symbol_id, quantity, blocked_qty, avg_buy_price, invested_value, first_buy_date)
SELECT u.user_id, s.symbol_id, 1500, 200, 250000, 375000000, CURRENT_DATE
FROM users u, symbols s
WHERE u.email = 'trader@valyrian.com' AND s.symbol = 'RELIANCE'
ON CONFLICT (user_id, symbol_id) DO NOTHING;

INSERT INTO user_holdings (user_id, symbol_id, quantity, blocked_qty, avg_buy_price, invested_value, first_buy_date)
SELECT u.user_id, s.symbol_id, 300, 0, 380000, 114000000, CURRENT_DATE
FROM users u, symbols s
WHERE u.email = 'trader@valyrian.com' AND s.symbol = 'TCS'
ON CONFLICT (user_id, symbol_id) DO NOTHING;

INSERT INTO user_holdings (user_id, symbol_id, quantity, blocked_qty, avg_buy_price, invested_value, first_buy_date)
SELECT u.user_id, s.symbol_id, 850, 0, 140000, 119000000, CURRENT_DATE
FROM users u, symbols s
WHERE u.email = 'trader@valyrian.com' AND s.symbol = 'HDFCBANK'
ON CONFLICT (user_id, symbol_id) DO NOTHING;

-- Generate some order history for Harsha
INSERT INTO order_history (user_id, symbol_id, side, type, status, price, quantity, created_at)
SELECT u.user_id, s.symbol_id, 'BUY', 'LIMIT', 'FILLED', 245000, 500, CURRENT_TIMESTAMP - interval '3 days'
FROM users u, symbols s
WHERE u.email = 'trader@valyrian.com' AND s.symbol = 'RELIANCE';

INSERT INTO order_history (user_id, symbol_id, side, type, status, price, quantity, created_at)
SELECT u.user_id, s.symbol_id, 'SELL', 'LIMIT', 'PENDING', 255000, 200, CURRENT_TIMESTAMP - interval '1 day'
FROM users u, symbols s
WHERE u.email = 'trader@valyrian.com' AND s.symbol = 'RELIANCE';

INSERT INTO order_history (user_id, symbol_id, side, type, status, price, quantity, created_at)
SELECT u.user_id, s.symbol_id, 'BUY', 'MARKET', 'FILLED', 380000, 300, CURRENT_TIMESTAMP - interval '2 days'
FROM users u, symbols s
WHERE u.email = 'trader@valyrian.com' AND s.symbol = 'TCS';

INSERT INTO order_history (user_id, symbol_id, side, type, status, price, quantity, created_at)
SELECT u.user_id, s.symbol_id, 'BUY', 'LIMIT', 'REJECTED', 130000, 1000, CURRENT_TIMESTAMP - interval '2 hours'
FROM users u, symbols s
WHERE u.email = 'trader@valyrian.com' AND s.symbol = 'HDFCBANK';

-- Update user funds for Harsha to show partial block
UPDATE user_funds 
SET blocked_funds = 51000000 -- 200 qt RELIANCE sell order margin/block mock
WHERE user_id IN (SELECT user_id FROM users WHERE email = 'trader@valyrian.com');
