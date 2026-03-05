#pragma once

#include "ports/EgressPort.h"
#include "queue/RingBuffer.h"
#include "model/OrderRequest.h"

namespace EMS {

class RingBufferEgress : public EgressPort {
public:
    explicit RingBufferEgress(RingBuffer<model::OrderRequest, 1024>& engine_queue);
    void forward(const model::EMSDecision& decision) override;

private:
    RingBuffer<model::OrderRequest, 1024>& queue_;
};

}