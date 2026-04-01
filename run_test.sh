#!/bin/bash
sudo -u postgres psql -d valyrian -c "INSERT INTO symbols (ticker, company_name) VALUES ('AAPL', 'Apple Inc.') ON CONFLICT DO NOTHING;"
echo "Starting Server..."
./build/API/APIServer &
SERVER_PID=$!
sleep 2
echo "Curling API..."
curl -s http://127.0.0.1:3000/api/v1/symbols
echo "Shutting down Server..."
kill $SERVER_PID
