#include "PGWriter.h"

#include <libpq-fe.h>

PGWriter::PGWriter(std::string connString) : connString_(std::move(connString))
{
    conn_ = PQconnectdb(connString_.c_str());
    if (!conn_ || PQstatus(conn_) != CONNECTION_OK)
    {
        if (conn_)
            PQfinish(conn_);
        conn_ = nullptr;
    }
}

PGWriter::~PGWriter()
{
    if (conn_)
        PQfinish(conn_);
}

void PGWriter::writeBatch(const std::vector<DBEvent> &batch)
{
    if (!conn_)
        return;
    (void)batch;
}
