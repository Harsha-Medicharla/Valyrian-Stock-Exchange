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
    print(f"  -> Provisioning {name} ({email})...")
    requests.post(f"{API_URL}/v1/signup", json={"email": email, "name": name, "password": "password123"})
    resp = requests.post(f"{API_URL}/v1/sessions", json={"email": email, "password": "password123"})
    
    if resp.status_code not in (200, 201):
        print(f"     [FATAL] Login failed! Status: {resp.status_code}")
        sys.exit(1)
        
    headers = {"Authorization": f"Bearer {resp.json().get('token')}"}
    if usd > 0:
        requests.post(f"{API_URL}/v1/account/deposit", json={"amount": usd}, headers=headers)
    if shares > 0:
        requests.post(f"{API_URL}/v1/account/deposit-holdings", json={"symbol_id": 1, "qty": shares}, headers=headers)
    return headers

async def run_simulation():
    print("--- Valyrian Stock Exchange: Advanced Mechanics Test ---\n")
    
    run_id = int(time.time())
    print("1. Provisioning 3 Independent Traders...")
    alice_headers = setup_account(f"alice_{run_id}@vse.com", "Alice (Seller)", 0, 1000)
    bob_headers = setup_account(f"bob_{run_id}@vse.com", "Bob (Seller)", 0, 1000)
    charlie_headers = setup_account(f"charlie_{run_id}@vse.com", "Charlie (Buyer)", 500000, 0)
    
    await asyncio.sleep(1) 

    print("\n2. Connecting WebSockets...")
    async with websockets.connect(WS_URL, additional_headers=alice_headers) as ws_alice, \
               websockets.connect(WS_URL, additional_headers=bob_headers) as ws_bob, \
               websockets.connect(WS_URL, additional_headers=charlie_headers) as ws_charlie:
        
        async def listen(ws, trader_name):
            try:
                async for msg in ws:
                    report = VSE.ExecutionReport.ExecutionReport.GetRootAsExecutionReport(msg, 0)
                    e_type = EVENT_TYPES.get(report.EventType(), f"UNKNOWN")
                    print(f"<<< [{trader_name}] Order {report.OrderId()} -> {e_type} | Fill Px: {report.FillPrice()}, Fill Qty: {report.FillQty()}, Remaining: {report.Remaining()}")
            except websockets.exceptions.ConnectionClosed:
                pass

        tasks = [
            asyncio.create_task(listen(ws_alice, "ALICE")),
            asyncio.create_task(listen(ws_bob, "BOB")),
            asyncio.create_task(listen(ws_charlie, "CHARLIE"))
        ]
        await asyncio.sleep(0.5)

        print("\n=== TEST 1: FIFO Priority & Partial Fills ===")
        print(">>> [ALICE] Places LIMIT SELL: 50 shares @ $1000")
        await ws_alice.send(create_new_order_buffer(1, 1, 1, 0, 1000, 50))
        await asyncio.sleep(0.5)

        print(">>> [BOB] Places LIMIT SELL: 50 shares @ $1000")
        await ws_bob.send(create_new_order_buffer(2, 1, 1, 0, 1000, 50))
        await asyncio.sleep(0.5)

        print("\n>>> [CHARLIE] Places LIMIT BUY: 25 shares @ $1000")
        await ws_charlie.send(create_new_order_buffer(3, 1, 0, 0, 1000, 25))
        await asyncio.sleep(2)

        print("\n=== TEST 2: Market Order Sweep & Liquidity Exhaustion ===")
        print(">>> [ALICE] Places LIMIT SELL: 20 shares @ $1010")
        await ws_alice.send(create_new_order_buffer(4, 1, 1, 0, 1010, 20))
        await asyncio.sleep(0.5)

        print(">>> [BOB] Places LIMIT SELL: 20 shares @ $1020")
        await ws_bob.send(create_new_order_buffer(5, 1, 1, 0, 1020, 20))
        await asyncio.sleep(0.5)

        print("\n>>> [CHARLIE] Places MARKET BUY: 100 shares")
        await ws_charlie.send(create_new_order_buffer(6, 1, 0, 1, 0, 100))
        await asyncio.sleep(2)

        for t in tasks: t.cancel()
        print("\nAdvanced Testing Complete!")

if __name__ == "__main__":
    asyncio.run(run_simulation())
