#!/bin/bash
echo "--- Logging In ---"
LOGIN_RES=$(curl -s -X POST http://127.0.0.1:3000/api/v1/auth/login -d '{"email":"trader@valyrian.com","password":"valyrian123"}')
echo "$LOGIN_RES"

TOKEN=$(echo "$LOGIN_RES" | grep -oP '(?<="token":")[^"]+')
USER_ID=$(echo "$LOGIN_RES" | grep -oP '(?<="user_id":)\d+')

echo -e "\n--- Funds ---"
curl -s -H "Authorization: Bearer $TOKEN" http://127.0.0.1:3000/api/v1/user/funds/$USER_ID

echo -e "\n\n--- Holdings ---"
curl -s -H "Authorization: Bearer $TOKEN" http://127.0.0.1:3000/api/v1/user/holdings/$USER_ID

echo -e "\n\n--- History ---"
curl -s -H "Authorization: Bearer $TOKEN" http://127.0.0.1:3000/api/v1/user/history/$USER_ID

echo -e "\n\n--- Symbols ---"
curl -s http://127.0.0.1:3000/api/v1/symbols
