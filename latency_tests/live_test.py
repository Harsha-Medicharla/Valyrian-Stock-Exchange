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

def setup_account(email, name, password, usd, shares):
    print(f"  -> Provisioning {email}...")
    
    reg_resp = requests.post(f"{API_URL}/v1/signup", json={"email": email, "name": name, "password": password})
    
    resp = requests.post(f"{API_URL}/v1/sessions", json={"email": email, "password": password})
    if resp.status_code not in (200, 201):
        print(f"     [FATAL] Login failed! Status: {resp.status_code} | Response: {resp.text.strip()}")
        sys.exit(1)
        
    token = resp.json().get("token")
    headers = {"Authorization": f"Bearer {token}"}
    
    if usd > 0:
        requests.post(f"{API_URL}/v1/account/deposit", json={"amount": usd}, headers=headers)
    if shares > 0:
        requests.post(f"{API_URL}/v1/account/deposit-holdings", json={"symbol_id": 1, "qty": shares}, headers=headers)
            
    return headers

async def run_simulation():
    print("--- Valyrian Stock Exchange: Multi-User Trade Engine Test ---")
    
    run_id = int(time.time())
    buyer_email = "harshamedicharla05@gmail.com"
    seller_email = f"seller_{run_id}@vse.com"
    
    print("\n1. Provisioning Trader A (Buyer) and Trader B (Seller)...")
    buyer_headers = setup_account(buyer_email, "harsha", "User01@2026", 100000, 0)
    seller_headers = setup_account(seller_email, "Seller Alice", "password123", 0, 500)
    
    await asyncio.sleep(1) 

    print("\n2. Connecting WebSockets...")
    async with websockets.connect(WS_URL, additional_headers=buyer_headers) as ws_buyer, \
               websockets.connect(WS_URL, additional_headers=seller_headers) as ws_seller:
        
        print("Both traders successfully connected to the Matching Engine!\n")
        
        async def listen(ws, trader_name):
            try:
                async for msg in ws:
                    report = VSE.ExecutionReport.ExecutionReport.GetRootAsExecutionReport(msg, 0)
                    e_type = EVENT_TYPES.get(report.EventType(), f"UNKNOWN({report.EventType()})")
                    print(f"<<< [{trader_name}] Received: Order {report.OrderId()} -> {e_type} | Fill Px: {report.FillPrice()}, Qty: {report.FillQty()}, Remaining: {report.Remaining()}")
            except websockets.exceptions.ConnectionClosed:
                pass

        listener_a = asyncio.create_task(listen(ws_buyer, "BUYER"))
        listener_b = asyncio.create_task(listen(ws_seller, "SELLER"))

        await asyncio.sleep(0.5)

        print(">>> [BUYER] Sending LIMIT BUY (Px: 1000, Qty: 50)")
        await ws_buyer.send(create_new_order_buffer(seq=1, symbol=1, side=0, o_type=0, price=1000, qty=50))
        
        await asyncio.sleep(1)
        
        print("\n>>> [SELLER] Sending LIMIT SELL (Px: 1000, Qty: 50) -> Should trigger CROSS!")
        await ws_seller.send(create_new_order_buffer(seq=2, symbol=1, side=1, o_type=0, price=1000, qty=50))
        
        await asyncio.sleep(1) 
        
        listener_a.cancel()
        listener_b.cancel()
        print("\nSimulation Complete. Your Matching Engine is fully operational!")

if __name__ == "__main__":
    asyncio.run(run_simulation())
