#!/bin/bash
echo "Starting Server..."
./build/API/APIServer &
SERVER_PID=$!
sleep 2
echo "--- GET /api/v1/symbols ---"
curl -s http://127.0.0.1:3000/api/v1/symbols
echo ""
echo "--- GET /api/v1/user/funds/1 ---"
curl -s http://127.0.0.1:3000/api/v1/user/funds/1
echo ""
echo "--- GET /api/v1/user/info/1 ---"
curl -s http://127.0.0.1:3000/api/v1/user/info/1
echo ""
echo "Shutting down Server..."
kill $SERVER_PID
