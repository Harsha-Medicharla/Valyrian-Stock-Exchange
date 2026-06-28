#pragma once
#include <vector>

#include "shared/types/Events.h"

class BatchBuffer
{
private:
    std::vector<DBEvent> events_;
    static constexpr std::size_t FLUSH_SIZE = 500;

public:
    explicit BatchBuffer() { events_.reserve(1000); }

    void push(const DBEvent &e) { events_.push_back(e); }

    [[nodiscard]] bool shouldFlush() const noexcept { return events_.size() >= FLUSH_SIZE; }

    [[nodiscard]] const std::vector<DBEvent> &events() const noexcept { return events_; }

    void clear() noexcept { events_.clear(); }
};
