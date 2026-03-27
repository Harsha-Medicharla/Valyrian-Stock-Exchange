INSERT INTO symbols (ticker, company_name, tick_size, lot_size, is_active) VALUES
    ('AAPL', 'Apple Inc',       1, 1, true),
    ('GOOG', 'Alphabet Inc',    1, 1, true),
    ('INFY', 'Infosys Ltd',     1, 1, true),
    ('RELIANCE', 'Reliance Industries', 1, 1, true)
ON CONFLICT (ticker) DO NOTHING;
