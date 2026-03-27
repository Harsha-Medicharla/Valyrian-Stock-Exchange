import asyncio
import websockets
import requests
import flatbuffers
import sys
import time

try:
    import VSE.Request
    import VSE.NewOrder
    import VSE.RequestBody
    import VSE.ExecutionReport
except ImportError:
    print("ERROR: VSE Flatbuffer Python classes not found!")
    sys.exit(1)

API_URL = "http://localhost:8080"
WS_URL = "ws://localhost:9001"
EVENT_TYPES = {0: "FILL", 1: "REJECT", 2: "CANCEL_ACK", 3: "MODIFY_ACK", 4: "NEW_ACK"}

def create_new_order_buffer(seq, symbol, side, o_type, price, qty):
    builder = flatbuffers.Builder(1024)
    VSE.NewOrder.Start(builder)
    VSE.NewOrder.AddClientSeq(builder, seq)
    VSE.NewOrder.AddSymbolId(builder, symbol)
    VSE.NewOrder.AddSide(builder, side)
    VSE.NewOrder.AddOrderType(builder, o_type)
    VSE.NewOrder.AddPrice(builder, price)
    VSE.NewOrder.AddQty(builder, qty)
    new_order_offset = VSE.NewOrder.End(builder)
    
    VSE.Request.Start(builder)
    VSE.Request.AddBodyType(builder, VSE.RequestBody.RequestBody().NewOrder)
    VSE.Request.AddBody(builder, new_order_offset)
    request_offset = VSE.Request.End(builder)
    
    builder.Finish(request_offset)
    return builder.Output()

def setup_account(email, name, usd, shares):
    print(f"  -> Provisioning {name}...")
    requests.post(f"{API_URL}/v1/signup", json={"email": email, "name": name, "password": "password123"})
    resp = requests.post(f"{API_URL}/v1/sessions", json={"email": email, "password": "password123"})
    headers = {"Authorization": f"Bearer {resp.json().get('token')}"}
    
    if usd > 0:
        requests.post(f"{API_URL}/v1/account/deposit", json={"amount": usd}, headers=headers)
    if shares > 0:
        requests.post(f"{API_URL}/v1/account/deposit-holdings", json={"symbol_id": 1, "qty": shares}, headers=headers)
    return headers

async def run_simulation():
    print("--- Valyrian Stock Exchange: Clean Slate Edge Cases ---\n")
    
    run_id = int(time.time())
    alice_headers = setup_account(f"alice_{run_id}@vse.com", "Alice", 500000, 1000)
    bob_headers = setup_account(f"bob_{run_id}@vse.com", "Bob", 500000, 1000)
    charlie_headers = setup_account(f"charlie_{run_id}@vse.com", "Charlie", 500000, 1000)
    
    await asyncio.sleep(1.5) 

    async with websockets.connect(WS_URL, additional_headers=alice_headers) as ws_alice, \
               websockets.connect(WS_URL, additional_headers=bob_headers) as ws_bob, \
               websockets.connect(WS_URL, additional_headers=charlie_headers) as ws_charlie:
        
        async def listen(ws, trader_name):
            try:
                async for msg in ws:
                    report = VSE.ExecutionReport.ExecutionReport.GetRootAsExecutionReport(msg, 0)
                    e_type = EVENT_TYPES.get(report.EventType(), f"UNKNOWN")
                    print(f"<<< [{trader_name}] Order {report.OrderId()} -> {e_type} | Px: {report.FillPrice()}, Qty: {report.FillQty()}, Rem: {report.Remaining()}")
            except: pass

        tasks = [
            asyncio.create_task(listen(ws_alice, "ALICE")),
            asyncio.create_task(listen(ws_bob, "BOB")),
            asyncio.create_task(listen(ws_charlie, "CHARLIE"))
        ]
        await asyncio.sleep(0.5)

        print("\n=== TEST 1: Price Priority (Queue Jumping) ===")
        print(">>> [ALICE] Places LIMIT BUY: 50 @ $1000 (First in time)")
        await ws_alice.send(create_new_order_buffer(1, 1, 0, 0, 1000, 50))
        await asyncio.sleep(0.5)

        print(">>> [BOB] Places LIMIT BUY: 50 @ $1010 (Better Price!)")
        await ws_bob.send(create_new_order_buffer(2, 1, 0, 0, 1010, 50))
        await asyncio.sleep(0.5)

        print("\n>>> [CHARLIE] Places LIMIT SELL: 50 @ $1000")
        await ws_charlie.send(create_new_order_buffer(3, 1, 1, 0, 1000, 50))
        await asyncio.sleep(2)

        print("\n=== TEST 2: The Flash Crash (Market Slippage) ===")
        print(">>> [ALICE] Places LIMIT BUY: 10 @ $900")
        await ws_alice.send(create_new_order_buffer(4, 1, 0, 0, 900, 10))
        await asyncio.sleep(0.5)

        print(">>> [BOB] Places LIMIT BUY: 10 @ $800")
        await ws_bob.send(create_new_order_buffer(5, 1, 0, 0, 800, 10))
        await asyncio.sleep(0.5)

        print("\n>>> [CHARLIE] Places MARKET SELL: 30 shares (Into a 20-share book)")
        await ws_charlie.send(create_new_order_buffer(6, 1, 1, 1, 0, 30))
        await asyncio.sleep(2)

        print("\n=== TEST 3: Wash Trade Interception ===")
        print(">>> [ALICE] Places LIMIT SELL: 50 @ $2000")
        await ws_alice.send(create_new_order_buffer(7, 1, 1, 0, 2000, 50))
        await asyncio.sleep(0.5)

        print(">>> [ALICE] Tries to cross herself - Places LIMIT BUY: 50 @ $2000")
        await ws_alice.send(create_new_order_buffer(8, 1, 0, 0, 2000, 50))
        await asyncio.sleep(2)

        for t in tasks: t.cancel()
        print("\nTesting Complete!")

if __name__ == "__main__":
    asyncio.run(run_simulation())
