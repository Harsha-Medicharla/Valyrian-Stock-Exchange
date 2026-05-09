#include <chrono>
#include <cstdint>
#include <thread>
#include "core/EMSCore.h"
#include "core/RejectHandler.h"
#include "types/Common.h"

static void onRejectCallback(uint32_t /*userId*/, RejectReason /*reason*/) noexcept
{
    // Intentionally empty: stub for production integration.
}

int main()
{
    // Register global reject callback.
    RejectHandler::registerCallback(&onRejectCallback);

    // Create a small EMS instance and exercise lifecycle.
    EMSCore ems(/*numWorkers=*/8, /*numSymbols=*/16);
    ems.start();

    // market open in this time
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ems.stop();
    ems.join();

    return 0;
}
