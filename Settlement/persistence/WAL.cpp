#include "WAL.h"
#include <unistd.h>
#include <fcntl.h>
#include <chrono>

WAL::WAL(const std::string& file) : filename(file), running(true) {
    out.open(filename, std::ios::binary | std::ios::app);
    fsync_thread = std::thread(&WAL::fsyncLoop, this);
}

WAL::~WAL() {
    running = false;
    if (fsync_thread.joinable()) {
        fsync_thread.join();
    }
}

uint32_t WAL::checksum(const Trade& t) {
    return t.trade_id ^ t.buyer_id ^ t.seller_id ^
           t.symbol ^ t.price ^ t.qty;
}

bool WAL::log(const Trade& t) {
    std::lock_guard<std::mutex> lock(mtx);

    uint32_t size = sizeof(Trade);
    uint32_t chk = checksum(t);

    out.write((char*)&size, sizeof(size));
    out.write((char*)&t, sizeof(t));
    out.write((char*)&chk, sizeof(chk));

    // ❌ NO fsync here anymore
    return true;
}

void WAL::fsyncLoop() {
    int fd = ::open(filename.c_str(), O_WRONLY);

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        {
            std::lock_guard<std::mutex> lock(mtx);
            out.flush();
        }

        if (fd >= 0) {
            fsync(fd);
        }
    }

    if (fd >= 0) {
        fsync(fd);
        close(fd);
    }
}

void WAL::replay(std::function<void(const Trade&)> callback) {
    std::ifstream in(filename, std::ios::binary);

    while (true) {
        uint32_t size;
        Trade t;
        uint32_t chk;

        if (!in.read((char*)&size, sizeof(size))) break;
        if (!in.read((char*)&t, sizeof(t))) break;
        if (!in.read((char*)&chk, sizeof(chk))) break;

        if (checksum(t) == chk) {
            callback(t);
        }
    }
}