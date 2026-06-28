#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <memory>
#include "OrderBook.h"
#include "MatchingEngine.h"

// Custom Statistic: Calculate P99 Tail Latency
auto P99 = [](const std::vector<double> &v) -> double
{
    if (v.empty())
        return 0.0;
    std::vector<double> copy = v;
    std::sort(copy.begin(), copy.end());
    size_t idx = static_cast<size_t>(0.99 * copy.size());
    return copy[idx];
};

// ========================================================================
// BENCHMARK 1: Pure Memory Pool (Zero Allocation Hot-Path)
// Measures the exact latency of popping/pushing a pre-allocated pointer.
// ========================================================================
static void BM_1_MemoryPool_AllocFree(benchmark::State &state)
{
    auto book = std::make_unique<OrderBook>();
    for (auto _ : state)
    {
        Order *o = book->requestAllocationOfOrder();
        benchmark::DoNotOptimize(o);
        book->requestDeAllocationOfOrder(o);
    }
}
BENCHMARK(BM_1_MemoryPool_AllocFree)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

// ========================================================================
// BENCHMARK 2: Adaptive Radix Tree (ART) & Intrusive Lists
// Measures the L1 cache efficiency of traversing the tree and linked list.
// ========================================================================
static void BM_2_DataStructures_InsertRemove(benchmark::State &state)
{
    auto book = std::make_unique<OrderBook>();
    uint64_t id = 1;
    for (auto _ : state)
    {
        Order *o = book->requestAllocationOfOrder();
        if (!o)
        {
            state.SkipWithError("Pool exhausted");
            break;
        }
        *o = {id, 1, Side::BUY, OrderType::LIMIT, 100, 10, 10, 0, OrderState::NEW, nullptr, nullptr};

        book->insertOrder(o);

        state.PauseTiming();
        book->removeOrder(o);
        id++;
        state.ResumeTiming();
    }
}
BENCHMARK(BM_2_DataStructures_InsertRemove)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

// ========================================================================
// BENCHMARK 3: Full Engine Ingestion (The WAL Disk Bottleneck)
// Measures the actual hot-path from ingestion to Write-Ahead Log flush.
// ========================================================================
static void BM_3_Engine_WAL_Ingestion(benchmark::State &state)
{
    std::remove("engine_111.wal");
    std::remove("engine_111.trades");
    auto engine = std::make_unique<MatchingEngine>(111);
    uint64_t id = 1;
    for (auto _ : state)
    {
        engine->onNewOrder(id, 1, Side::BUY, OrderType::LIMIT, 100, 10, 0);

        state.PauseTiming();
        engine->onCancelOrder(id);
        id++;
        state.ResumeTiming();
    }
    std::remove("engine_111.wal");
    std::remove("engine_111.trades");
}
BENCHMARK(BM_3_Engine_WAL_Ingestion)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

// ========================================================================
// BENCHMARK 4: Engine Crossing (Matching & Execution)
// Measures the latency of crossing the spread and generating trade events.
// ========================================================================
static void BM_4_Engine_Cross_Spread(benchmark::State &state)
{
    std::remove("engine_222.wal");
    std::remove("engine_222.trades");
    auto engine = std::make_unique<MatchingEngine>(222);
    uint64_t id = 1;
    for (auto _ : state)
    {
        state.PauseTiming();
        engine->onNewOrder(id, 1, Side::SELL, OrderType::LIMIT, 100, 10, 0);
        uint64_t incoming_id = id + 1;
        state.ResumeTiming();

        engine->onNewOrder(incoming_id, 2, Side::BUY, OrderType::MARKET, 0, 10, 0);

        state.PauseTiming();
        id += 2;
        state.ResumeTiming();
    }
    std::remove("engine_222.wal");
    std::remove("engine_222.trades");
}
BENCHMARK(BM_4_Engine_Cross_Spread)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

// ========================================================================
// BENCHMARK 5: Deep Book Sweep (Liquidity Consumption)
// Measures performance when a massive order wipes out 5 price levels.
// ========================================================================
static void BM_5_Engine_Deep_Sweep(benchmark::State &state)
{
    std::remove("engine_333.wal");
    std::remove("engine_333.trades");
    auto engine = std::make_unique<MatchingEngine>(333);
    uint64_t id = 1;
    for (auto _ : state)
    {
        state.PauseTiming();
        for (int i = 0; i < 5; i++)
        {
            engine->onNewOrder(id++, 1, Side::SELL, OrderType::LIMIT, 100 + i, 10, 0);
        }
        uint64_t incoming_id = id++;
        state.ResumeTiming();

        engine->onNewOrder(incoming_id, 2, Side::BUY, OrderType::MARKET, 0, 50, 0);

        state.PauseTiming();
        state.ResumeTiming();
    }
    std::remove("engine_333.wal");
    std::remove("engine_333.trades");
}
BENCHMARK(BM_5_Engine_Deep_Sweep)->Repetitions(10)->ComputeStatistics("p99", P99)->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();
