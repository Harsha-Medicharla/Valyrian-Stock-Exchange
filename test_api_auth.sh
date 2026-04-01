#!/bin/bash
./build/API/APIServer &
SERVER_PID=$!
sleep 4

echo "--- Registering User ---"
curl -s -X POST http://127.0.0.1:3000/api/v1/auth/register -d '{"email":"test@example.com","password":"secret123","name":"Test User"}'
echo ""

echo "--- Logging In ---"
LOGIN_RES=$(curl -s -X POST http://127.0.0.1:3000/api/v1/auth/login -d '{"email":"test@example.com","password":"secret123"}')
echo $LOGIN_RES
TOKEN=$(echo $LOGIN_RES | grep -oP '(?<="token":")[^"]+')
USER_ID=$(echo $LOGIN_RES | grep -oP '(?<="user_id":)\d+')

echo "--- Accessing Protected Route (Missing Token) ---"
curl -s http://127.0.0.1:3000/api/v1/user/info/$USER_ID
echo ""

echo "--- Accessing Protected Route (With Token) ---"
curl -s -H "Authorization: Bearer $TOKEN" http://127.0.0.1:3000/api/v1/user/info/$USER_ID
echo ""

kill $SERVER_PID
