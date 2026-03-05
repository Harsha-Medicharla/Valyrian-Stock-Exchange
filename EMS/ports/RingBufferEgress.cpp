#include "ports/RingBufferEgress.h"

namespace EMS {

RingBufferEgress::RingBufferEgress(RingBuffer<model::OrderRequest, 1024>& engine_queue)
    : queue_(engine_queue) {}

void RingBufferEgress::forward(const model::EMSDecision& decision) {
    if (!decision.accepted) {
        return; 
    }
    queue_.push(decision.original_request);
}

}