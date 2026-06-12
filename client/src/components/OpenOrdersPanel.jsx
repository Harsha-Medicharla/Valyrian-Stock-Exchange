import { useState } from 'react'
import { useQuery, useQueryClient } from '@tanstack/react-query'
import api from '../api/axios'

function formatTime(iso) {
  const d = new Date(iso)
  return d.toLocaleString('en-IN', { hour12: false, timeZone: 'Asia/Kolkata' })
}

function fmt(n) {
  return Number(n).toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

export default function OpenOrdersPanel() {
  const queryClient = useQueryClient()
  const [cancellingId, setCancellingId] = useState(null)
  const [cancelError, setCancelError] = useState('')
  const [cancelSuccess, setCancelSuccess] = useState('')

  const { data: orders = [], isLoading } = useQuery({
    queryKey: ['my-orders'],
    queryFn: () => api.get('/api/orders/my').then(r => r.data),
    refetchInterval: 8000,
  })

  const openOrders = orders.filter(o => o.status === 'OPEN')
  const recentClosed = orders.filter(o => o.status !== 'OPEN').slice(0, 5)

  async function handleCancel(orderId) {
    setCancellingId(orderId)
    setCancelError('')
    setCancelSuccess('')
    try {
      await api.post('/api/orders/cancel', { orderId })
      setCancelSuccess(`Order #${orderId} cancelled.`)
      queryClient.invalidateQueries({ queryKey: ['my-orders'] })
      queryClient.invalidateQueries({ queryKey: ['portfolio'] })
    } catch (err) {
      setCancelError(err.response?.data?.error || 'Cancel failed.')
    } finally {
      setCancellingId(null)
    }
  }

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <p className="mono text-xs text-text-muted tracking-widest">MY OPEN ORDERS</p>
        {openOrders.length > 0 && (
          <span className="badge-warn">{openOrders.length} OPEN</span>
        )}
      </div>

      {/* Feedback messages */}
      {cancelSuccess && (
        <div className="mx-5 mt-3 px-3 py-2 bg-profit/10 border border-profit/30 rounded-sm">
          <p className="mono text-xs text-profit">{cancelSuccess}</p>
        </div>
      )}
      {cancelError && (
        <div className="mx-5 mt-3 px-3 py-2 bg-loss/10 border border-loss/30 rounded-sm">
          <p className="mono text-xs text-loss">{cancelError}</p>
        </div>
      )}

      {isLoading ? (
        <div className="p-5 space-y-2 animate-pulse">
          {[...Array(3)].map((_, i) => <div key={i} className="h-10 bg-border rounded-sm" />)}
        </div>
      ) : openOrders.length === 0 ? (
        <div className="px-5 py-8 text-center">
          <p className="mono text-xs text-text-muted">NO OPEN ORDERS</p>
          <p className="mono text-[10px] text-text-muted mt-1">Place a LIMIT order to see it here.</p>
        </div>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-sm min-w-[620px]">
            <thead>
              <tr className="border-b border-border">
                {['#', 'Asset', 'Side', 'Type', 'Qty', 'Filled', 'Limit Price', 'Placed', 'Action'].map(h => (
                  <th key={h} className="text-left px-4 py-3 mono text-[10px] text-text-muted tracking-wider">
                    {h}
                  </th>
                ))}
              </tr>
            </thead>
            <tbody>
              {openOrders.map(o => (
                <tr key={o.id} className="border-b border-border hover:bg-bg/50 transition-colors">
                  <td className="px-4 py-3 mono text-xs text-text-muted">{o.id}</td>
                  <td className="px-4 py-3 mono text-xs text-info font-medium">{o.assetSymbol}</td>
                  <td className="px-4 py-3">
                    <span className={o.type === 'BUY' ? 'badge-profit' : 'badge-loss'}>{o.type}</span>
                  </td>
                  <td className="px-4 py-3 mono text-xs text-text-muted">{o.order_type}</td>
                  <td className="px-4 py-3 mono text-xs text-text-primary">{Number(o.qty).toFixed(0)}</td>
                  <td className="px-4 py-3 mono text-xs text-text-muted">
                    {Number(o.filled_qty ?? 0).toFixed(0)} / {Number(o.qty).toFixed(0)}
                  </td>
                  <td className="px-4 py-3 mono text-xs text-text-primary">
                    {o.limit_price ? `₹${fmt(o.limit_price)}` : <span className="text-text-muted">MARKET</span>}
                  </td>
                  <td className="px-4 py-3 mono text-[10px] text-text-muted whitespace-nowrap">
                    {formatTime(o.created_at)}
                  </td>
                  <td className="px-4 py-3">
                    <button
                      onClick={() => handleCancel(o.id)}
                      disabled={cancellingId === o.id}
                      className="mono text-[10px] px-3 py-1.5 border border-loss/40 text-loss rounded-sm hover:bg-loss/10 transition-colors disabled:opacity-40 disabled:cursor-not-allowed"
                    >
                      {cancellingId === o.id ? 'CANCELLING...' : 'CANCEL'}
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}

      {/* Recently Closed Orders */}
      {recentClosed.length > 0 && (
        <div className="border-t border-border">
          <div className="px-5 py-3 border-b border-border">
            <p className="mono text-[10px] text-text-muted tracking-widest">RECENTLY CLOSED</p>
          </div>
          <div className="overflow-x-auto">
            <table className="w-full text-sm min-w-[520px]">
              <tbody>
                {recentClosed.map(o => (
                  <tr key={o.id} className="border-b border-border opacity-60">
                    <td className="px-4 py-2 mono text-xs text-text-muted">#{o.id}</td>
                    <td className="px-4 py-2 mono text-xs text-info">{o.assetSymbol}</td>
                    <td className="px-4 py-2">
                      <span className={o.type === 'BUY' ? 'badge-profit' : 'badge-loss'}>{o.type}</span>
                    </td>
                    <td className="px-4 py-2 mono text-xs text-text-muted">{Number(o.qty).toFixed(0)} qty</td>
                    <td className="px-4 py-2">
                      <span className={`badge-${o.status === 'FILLED' ? 'profit' : 'warn'}`}>{o.status}</span>
                    </td>
                    <td className="px-4 py-2 mono text-[10px] text-text-muted whitespace-nowrap">
                      {formatTime(o.created_at)}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}
    </div>
  )
}
