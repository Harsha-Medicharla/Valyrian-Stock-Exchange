import { useState } from 'react'
import { useQueryClient } from '@tanstack/react-query'
import api from '../api/axios'

export default function OrderForm({
  assets = [],
  isLoading = false,
  side,
  orderType,
  assetId,
  qty,
  limitPrice,
  onSideChange,
  onOrderTypeChange,
  onAssetChange,
  onQtyChange,
  onLimitPriceChange,
  onTradeSuccess,
}) {
  const queryClient = useQueryClient()
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  const selectedAsset = assets.find((a) => a.assetId === assetId) || assets[0]

  function isValid() {
    const q = parseFloat(qty)
    if (!q || q <= 0) return false
    if (orderType === 'LIMIT') {
      const lp = parseFloat(limitPrice)
      if (!lp || lp <= 0) return false
    }
    return true
  }

  async function handleSubmit(e) {
    e.preventDefault()
    if (!qty || isNaN(qty) || Number(qty) <= 0) return

    setLoading(true)
    setError('')

    try {
      const res = await api.post('/api/trades', {
        assetId,
        type: side,
        orderType,
        qty: Number(qty),
        limitPrice: orderType === 'LIMIT' ? Number(limitPrice) : null
      })

      // Refetch portfolio and audit logs to update the UI
      queryClient.invalidateQueries({ queryKey: ['portfolio'] })
      queryClient.invalidateQueries({ queryKey: ['audit'] })
      queryClient.invalidateQueries({ queryKey: ['trade-history'] })

      if (onTradeSuccess) {
        onTradeSuccess(res.data, side, selectedAsset)
      }

      // Reset quantity
      onQtyChange('')
      onLimitPriceChange('')
    } catch (err) {
      setError(err.response?.data?.error || 'Trade execution failed.')
    } finally {
      setLoading(false)
    }
  }

  if (isLoading) {
    return (
      <div className="surface rounded-sm overflow-hidden">
        <div className="px-5 py-3 border-b border-border">
          <p className="mono text-xs text-text-muted tracking-widest">PLACE ORDER</p>
        </div>
        <div className="p-5 space-y-4 animate-pulse">
          {[1, 2, 3].map((i) => (
            <div key={i} className="h-10 bg-border rounded-sm" />
          ))}
        </div>
      </div>
    )
  }

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">PLACE ORDER</p>
      </div>

      <form onSubmit={handleSubmit} className="p-5 space-y-4">
        <div className="grid grid-cols-2 gap-2">
          {['BUY', 'SELL'].map((s) => (
            <button
              key={s}
              type="button"
              onClick={() => onSideChange(s)}
              className={`mono text-xs font-medium py-2.5 tracking-widest rounded-sm border transition-colors ${side === s
                ? s === 'BUY'
                  ? 'bg-profit/10 border-profit text-profit'
                  : 'bg-loss/10 border-loss text-loss'
                : 'border-border text-text-muted hover:text-text-primary'
                }`}
            >
              {s}
            </button>
          ))}
        </div>

        <div className="grid grid-cols-2 gap-2">
          {['MARKET', 'LIMIT'].map((ot) => (
            <button
              key={ot}
              type="button"
              onClick={() => onOrderTypeChange(ot)}
              className={`mono text-xs py-2 tracking-widest rounded-sm border transition-colors ${orderType === ot
                ? 'border-info text-info bg-info/10'
                : 'border-border text-text-muted hover:text-text-primary'
                }`}
            >
              {ot}
            </button>
          ))}
        </div>

        <div>
          <label className="block mono text-xs text-text-muted mb-1.5 tracking-wider">ASSET</label>
          <select
            value={assetId ?? ''}
            onChange={(e) => onAssetChange(parseInt(e.target.value))}
            className="w-full px-3 py-2.5 text-sm rounded-sm mono"
          >
            {assets.map((a) => (
              <option key={a.assetId} value={a.assetId}>
                {a.symbol} — {a.name}
              </option>
            ))}
          </select>
        </div>

        <div>
          <label className="block mono text-xs text-text-muted mb-1.5 tracking-wider">QUANTITY</label>
          <input
            type="number"
            min="1"
            step="1"
            value={qty}
            onChange={(e) => onQtyChange(e.target.value)}
            placeholder="0"
            className="w-full px-3 py-2.5 text-sm rounded-sm mono"
          />
        </div>

        {orderType === 'LIMIT' && (
          <div>
            <label className="block mono text-xs text-text-muted mb-1.5 tracking-wider">
              LIMIT PRICE (₹)
            </label>
            <input
              type="number"
              min="0.01"
              step="0.01"
              value={limitPrice}
              onChange={(e) => onLimitPriceChange(e.target.value)}
              placeholder={selectedAsset ? selectedAsset.currentPrice.toFixed(2) : '0.00'}
              className="w-full px-3 py-2.5 text-sm rounded-sm mono"
            />
          </div>
        )}

        {error && <p className="mono text-xs text-loss">{error}</p>}

        <button
          type="submit"
          disabled={!isValid() || loading}
          className={`w-full mono text-sm font-medium py-3 rounded-sm tracking-widest transition-colors disabled:opacity-40 disabled:cursor-not-allowed ${side === 'BUY'
            ? 'bg-profit text-black hover:bg-green-400'
            : 'bg-loss text-white hover:bg-red-400'
            }`}
        >
          {loading ? 'EXECUTING...' : `PLACE ${side} ORDER →`}
        </button>
      </form>
    </div>
  )
}
