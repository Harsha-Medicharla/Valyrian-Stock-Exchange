#include "PGPool.h"

#include <cstdlib>
#include <string>

#include <drogon/drogon.h>

namespace
{
    struct PgConnParts
    {
        std::string host = "127.0.0.1";
        uint16_t port = 5432;
        std::string dbname = "postgres";
        std::string user = "postgres";
        std::string password;
    };

    [[nodiscard]] std::string connValue(const std::string &connInfo, const std::string &key)
    {
        const std::string needle = key + "=";
        const std::size_t start = connInfo.find(needle);
        if (start == std::string::npos)
            return {};
        const std::size_t valueStart = start + needle.size();
        const std::size_t end = connInfo.find(' ', valueStart);
        return connInfo.substr(valueStart, end == std::string::npos ? std::string::npos : end - valueStart);
    }

    [[nodiscard]] PgConnParts parseConnInfo(const std::string &connInfo)
    {
        PgConnParts parts;
        if (const std::string host = connValue(connInfo, "host"); !host.empty())
            parts.host = host;
        if (const std::string port = connValue(connInfo, "port"); !port.empty())
            parts.port = static_cast<uint16_t>(std::atoi(port.c_str()));
        if (const std::string dbname = connValue(connInfo, "dbname"); !dbname.empty())
            parts.dbname = dbname;
        if (const std::string user = connValue(connInfo, "user"); !user.empty())
            parts.user = user;
        if (const std::string password = connValue(connInfo, "password"); !password.empty())
            parts.password = password;
        return parts;
    }
}

void PGPool::init(const std::string &connInfo, size_t connNum)
{
    const PgConnParts parts = parseConnInfo(connInfo);
    drogon::app().createDbClient(
        "postgresql",
        parts.host,
        parts.port,
        parts.dbname,
        parts.user,
        parts.password,
        connNum,
        "default");
}

drogon::orm::DbClientPtr PGPool::client()
{
    return drogon::app().getDbClient("default");
}
