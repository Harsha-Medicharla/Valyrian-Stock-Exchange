import { useQuery } from '@tanstack/react-query'
import api from '../api/axios'

function fmt(n) {
  return Number(n).toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

export default function OrderBook({ selectedAssetId, assets = [] }) {
  const assetId = selectedAssetId ?? assets[0]?.assetId
  const asset = assets.find(a => a.assetId === assetId)

  const { data: book = [], isLoading } = useQuery({
    queryKey: ['order-book', assetId],
    queryFn: () =>
      assetId
        ? api.get(`/api/orders/book/${assetId}`).then(r => r.data)
        : Promise.resolve([]),
    enabled: !!assetId,
    refetchInterval: 5000,
  })

  const bids = book.filter(o => o.side === 'BUY').slice(0, 8)
  const asks = book.filter(o => o.side === 'SELL').slice(0, 8)

  const maxBidQty = Math.max(...bids.map(o => Number(o.remaining_qty)), 1)
  const maxAskQty = Math.max(...asks.map(o => Number(o.remaining_qty)), 1)

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <p className="mono text-xs text-text-muted tracking-widest">ORDER BOOK</p>
        <div className="flex items-center gap-2">
          {asset && (
            <span className="mono text-xs text-info font-medium">{asset.symbol}</span>
          )}
          <div className="w-1.5 h-1.5 rounded-full bg-profit animate-pulse" />
          <span className="mono text-[10px] text-profit tracking-widest">LIVE</span>
        </div>
      </div>

      {isLoading ? (
        <div className="p-5 space-y-2 animate-pulse">
          {[...Array(6)].map((_, i) => (
            <div key={i} className="h-5 bg-border rounded-sm" />
          ))}
        </div>
      ) : (
        <div className="grid grid-cols-2 divide-x divide-border">
          {/* BUY side — Bids */}
          <div>
            <div className="px-4 py-2 border-b border-border grid grid-cols-3 gap-2">
              <span className="mono text-[10px] text-text-muted tracking-wider">PRICE</span>
              <span className="mono text-[10px] text-text-muted tracking-wider text-right">QTY</span>
              <span className="mono text-[10px] text-text-muted tracking-wider text-right">REMAIN</span>
            </div>
            {bids.length === 0 ? (
              <p className="mono text-[10px] text-text-muted px-4 py-4 text-center">NO BIDS</p>
            ) : (
              bids.map(o => {
                const pct = (Number(o.remaining_qty) / maxBidQty) * 100
                return (
                  <div key={o.order_id} className="relative px-4 py-1.5 grid grid-cols-3 gap-2 overflow-hidden group">
                    {/* depth heat bar */}
                    <div
                      className="absolute inset-0 bg-profit/5 origin-left transition-all"
                      style={{ width: `${pct}%` }}
                    />
                    <span className="mono text-xs text-profit font-medium relative z-10">
                      ₹{fmt(o.limit_price)}
                    </span>
                    <span className="mono text-xs text-text-primary text-right relative z-10">
                      {Number(o.qty).toFixed(0)}
                    </span>
                    <span className="mono text-xs text-text-muted text-right relative z-10">
                      {Number(o.remaining_qty).toFixed(0)}
                    </span>
                  </div>
                )
              })
            )}
          </div>

          {/* SELL side — Asks */}
          <div>
            <div className="px-4 py-2 border-b border-border grid grid-cols-3 gap-2">
              <span className="mono text-[10px] text-text-muted tracking-wider">PRICE</span>
              <span className="mono text-[10px] text-text-muted tracking-wider text-right">QTY</span>
              <span className="mono text-[10px] text-text-muted tracking-wider text-right">REMAIN</span>
            </div>
            {asks.length === 0 ? (
              <p className="mono text-[10px] text-text-muted px-4 py-4 text-center">NO ASKS</p>
            ) : (
              asks.map(o => {
                const pct = (Number(o.remaining_qty) / maxAskQty) * 100
                return (
                  <div key={o.order_id} className="relative px-4 py-1.5 grid grid-cols-3 gap-2 overflow-hidden group">
                    <div
                      className="absolute inset-0 bg-loss/5 origin-left transition-all"
                      style={{ width: `${pct}%` }}
                    />
                    <span className="mono text-xs text-loss font-medium relative z-10">
                      ₹{fmt(o.limit_price)}
                    </span>
                    <span className="mono text-xs text-text-primary text-right relative z-10">
                      {Number(o.qty).toFixed(0)}
                    </span>
                    <span className="mono text-xs text-text-muted text-right relative z-10">
                      {Number(o.remaining_qty).toFixed(0)}
                    </span>
                  </div>
                )
              })
            )}
          </div>
        </div>
      )}

      {!isLoading && book.length === 0 && (
        <p className="mono text-xs text-text-muted text-center py-6">
          NO OPEN LIMIT ORDERS FOR THIS ASSET
        </p>
      )}

      <div className="px-5 py-2 border-t border-border">
        <p className="mono text-[10px] text-text-muted">
          LIMIT orders only · price-time priority · auto-refreshes every 5s
        </p>
      </div>
    </div>
  )
}
