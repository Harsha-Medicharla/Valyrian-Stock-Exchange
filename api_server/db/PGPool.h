#pragma once

#include <drogon/orm/DbClient.h>

#include <string>

class PGPool
{
public:
    static void init(const std::string &connInfo, size_t connNum = 4);
    static drogon::orm::DbClientPtr client();
};
