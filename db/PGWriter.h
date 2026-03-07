#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "shared/types/Events.h"

struct pg_conn;
using PGconn = struct pg_conn;

class IDBWriterBackend
{
public:
    virtual ~IDBWriterBackend() = default;
    virtual void writeBatch(const std::vector<DBEvent> &batch) = 0;
};

class PGWriter : public IDBWriterBackend
{
private:
    PGconn *conn_{nullptr};
    std::string connString_;
    std::atomic<std::uint64_t> nextTradeId_{1};

public:
    explicit PGWriter(std::string connString);
    ~PGWriter();

    PGWriter(const PGWriter &) = delete;
    PGWriter &operator=(const PGWriter &) = delete;

    void writeBatch(const std::vector<DBEvent> &batch) override;
};
