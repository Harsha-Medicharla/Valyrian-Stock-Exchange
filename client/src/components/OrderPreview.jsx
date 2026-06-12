function fmt(n) {
  return '₹' + Number(n).toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

export default function OrderPreview({
  side,
  orderType,
  assetId,
  qty,
  limitPrice,
  lastTrade,
  // Real data props (from DashboardPage via /api/portfolio)
  assets = [],
  portfolio = null,
}) {
  const asset = assets.find((a) => a.assetId === (assetId || assets[0]?.assetId)) || assets[0]
  const holding = portfolio?.holdings.find((h) => h.assetId === (assetId || assets[0]?.assetId))

  const price =
    orderType === 'LIMIT' && parseFloat(limitPrice) > 0
      ? parseFloat(limitPrice)
      : asset?.currentPrice ?? 0

  const total = (parseInt(qty) || 0) * price
  const isBuy = side === 'BUY'

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">ORDER PREVIEW</p>
      </div>

      <div className="p-5 space-y-3">
        {/* Last trade confirmation */}
        {lastTrade && (
          <div
            className={`border rounded-sm p-3 mb-4 ${
              lastTrade.side === 'BUY' ? 'border-profit/30 bg-profit/5' : 'border-loss/30 bg-loss/5'
            }`}
          >
            <p className="mono text-xs text-text-muted mb-1 tracking-wider">LAST EXECUTION</p>
            <p
              className={`mono text-sm font-medium ${
                lastTrade.side === 'BUY' ? 'text-profit' : 'text-loss'
              }`}
            >
              {lastTrade.side} {lastTrade.qty}x {lastTrade.symbol} COMMITTED
            </p>
            <p className="mono text-xs text-text-muted mt-0.5">
              Trade #{lastTrade.tradeId} · {fmt(lastTrade.executedPrice)}/share
            </p>
          </div>
        )}

        <div className="flex justify-between items-center py-2 border-b border-border">
          <span className="mono text-xs text-text-muted">Asset</span>
          <span className="mono text-xs text-info">{asset?.symbol ?? '—'}</span>
        </div>
        <div className="flex justify-between items-center py-2 border-b border-border">
          <span className="mono text-xs text-text-muted">Side</span>
          <span className={`mono text-xs font-medium ${isBuy ? 'text-profit' : 'text-loss'}`}>
            {side}
          </span>
        </div>
        <div className="flex justify-between items-center py-2 border-b border-border">
          <span className="mono text-xs text-text-muted">Order Type</span>
          <span className="mono text-xs text-text-primary">{orderType}</span>
        </div>
        <div className="flex justify-between items-center py-2 border-b border-border">
          <span className="mono text-xs text-text-muted">Est. Price</span>
          <span className="mono text-xs text-text-primary">{asset ? fmt(price) : '—'}</span>
        </div>
        <div className="flex justify-between items-center py-2 border-b border-border">
          <span className="mono text-xs text-text-muted">Quantity</span>
          <span className="mono text-xs text-text-primary">{parseInt(qty) || '—'}</span>
        </div>
        <div className="flex justify-between items-center py-2">
          <span className="mono text-xs text-text-muted">
            {isBuy ? 'Est. Cost' : 'Est. Proceeds'}
          </span>
          <span
            className={`mono text-base font-medium ${
              total > 0 ? (isBuy ? 'text-loss' : 'text-profit') : 'text-text-muted'
            }`}
          >
            {total > 0 ? fmt(total) : '—'}
          </span>
        </div>

        {/* Available capacity — real cash balance / real holding qty */}
        <div className="mt-2 pt-3 border-t border-border">
          <div className="flex justify-between">
            <span className="mono text-xs text-text-muted">
              {isBuy ? 'Cash Available' : 'Holdings Available'}
            </span>
            <span className="mono text-xs text-text-muted">
              {portfolio == null
                ? '—'
                : isBuy
                ? fmt(portfolio.cashBalance)
                : `${holding?.availableQty ?? holding?.qty ?? 0} shares`}
            </span>
          </div>
        </div>
      </div>
    </div>
  )
}
