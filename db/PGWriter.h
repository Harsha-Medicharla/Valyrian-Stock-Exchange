#pragma once
#include <string>
#include <vector>

#include "shared/types/Events.h"

struct pg_conn;
using PGconn = struct pg_conn;

class PGWriter
{
private:
    PGconn *conn_{nullptr};
    std::string connString_;

public:
    explicit PGWriter(std::string connString);
    ~PGWriter();

    PGWriter(const PGWriter &) = delete;
    PGWriter &operator=(const PGWriter &) = delete;

    void writeBatch(const std::vector<DBEvent> &batch);
};
