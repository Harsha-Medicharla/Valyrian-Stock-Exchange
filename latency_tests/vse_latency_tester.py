import asyncio
import aiohttp
import websockets
import time
import json
import statistics
import flatbuffers

import VSE.NewOrder
import VSE.Request
import VSE.RequestBody
import VSE.Side
import VSE.OrderType
import VSE.ExecutionReport

REST_URL = "http://127.0.0.1:8080"
WS_URL = "ws://127.0.0.1:9001"
ITERATIONS = 5
LOAD_PROFILES = [100, 1000, 5000]

# --- 1. REST API Testing ---
async def rest_worker(session, url, headers, payload, results_list):
    start_time = time.perf_counter_ns()
    async with session.post(url, headers=headers, json=payload) as resp:
        await resp.read()
    end_time = time.perf_counter_ns()
    results_list.append((end_time - start_time) / 1_000_000) # Convert to milliseconds

async def run_rest_load_test(token, load_count):
    headers = {"Authorization": f"Bearer {token}", "Content-Type": "application/json"}
    payload = {"amount": 10} # Simulating a small deposit
    url = f"{REST_URL}/v1/account/deposit"
    
    results = []
    async with aiohttp.ClientSession() as session:
        tasks = [rest_worker(session, url, headers, payload, results) for _ in range(load_count)]
        await asyncio.gather(*tasks)
    
    return results

# --- 2. WebSocket Binary Testing ---
def build_new_order_fbs(client_seq, symbol_id=1, price=100, qty=10):
    builder = flatbuffers.Builder(128)
    
    VSE.NewOrder.Start(builder)
    VSE.NewOrder.AddClientSeq(builder, client_seq)
    VSE.NewOrder.AddSymbolId(builder, symbol_id)
    VSE.NewOrder.AddSide(builder, VSE.Side.Side().BUY)
    VSE.NewOrder.AddOrderType(builder, VSE.OrderType.OrderType().LIMIT)
    VSE.NewOrder.AddPrice(builder, price)
    VSE.NewOrder.AddQty(builder, qty)
    new_order = VSE.NewOrder.End(builder)
    
    VSE.Request.Start(builder)
    VSE.Request.AddBodyType(builder, VSE.RequestBody.RequestBody().NewOrder)
    VSE.Request.AddBody(builder, new_order)
    req = VSE.Request.End(builder)
    
    builder.Finish(req)
    return builder.Output()

async def ws_load_test(token, load_count):
    headers = {"Authorization": f"Bearer {token}"}
    results = []
    
    async with websockets.connect(WS_URL, additional_headers=headers) as websocket:
        messages = [build_new_order_fbs(i) for i in range(1, load_count + 1)]
        
        start_time = time.perf_counter_ns()
        
        for msg in messages:
            await websocket.send(msg)
            
        try:
            for _ in range(load_count):
                await asyncio.wait_for(websocket.recv(), timeout=2.0)
        except asyncio.TimeoutError:
            pass
            
        end_time = time.perf_counter_ns()
        
        total_time_ms = (end_time - start_time) / 1_000_000
        avg_latency_ms = total_time_ms / load_count
        return avg_latency_ms

# --- Main Test Runner ---
async def get_token():
    async with aiohttp.ClientSession() as session:
        # Create user
        await session.post(f"{REST_URL}/v1/signup", json={"email": "test@vse.com", "password": "password123"})
        # Login
        async with session.post(f"{REST_URL}/v1/sessions", json={"email": "test@vse.com", "password": "password123"}) as resp:
            data = await resp.json()
            return data["token"]

async def main():
    print("Initializing Setup and fetching Auth Token...")
    token = await get_token()
    
    print("\n" + "="*50)
    print(" REST API LOAD TEST (/v1/account/deposit)")
    print("="*50)
    for load in LOAD_PROFILES:
        print(f"\n--- Testing Load: {load} Concurrent Requests ---")
        avg_latencies = []
        for i in range(ITERATIONS):
            latencies = await run_rest_load_test(token, load)
            avg = statistics.mean(latencies)
            p99 = statistics.quantiles(latencies, n=100)[-1]
            avg_latencies.append(avg)
            print(f"  Iteration {i+1}: Avg = {avg:.2f} ms | P99 = {p99:.2f} ms")
        print(f">>> FINAL AVG for {load} reqs: {statistics.mean(avg_latencies):.2f} ms")

    print("\n" + "="*50)
    print(" WEBSOCKET MATCHING ENGINE LOAD TEST (FlatBuffers)")
    print("="*50)
    for load in LOAD_PROFILES:
        print(f"\n--- Testing Load: {load} Concurrent Orders ---")
        avg_latencies = []
        for i in range(ITERATIONS):
            avg_latency = await ws_load_test(token, load)
            avg_latencies.append(avg_latency)
            print(f"  Iteration {i+1}: Avg RTT per order = {avg_latency:.4f} ms")
        print(f">>> FINAL AVG for {load} orders: {statistics.mean(avg_latencies):.4f} ms")

if __name__ == "__main__":
    asyncio.run(main())
