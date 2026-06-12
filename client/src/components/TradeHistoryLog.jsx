import { useQuery } from '@tanstack/react-query'
import api from '../api/axios'

function formatTime(iso) {
  const d = new Date(iso)
  return d.toLocaleString('en-IN', { hour12: false, timeZone: 'Asia/Kolkata' })
}

export default function TradeHistoryLog() {
  const { data: history, isLoading } = useQuery({
    queryKey: ['trade-history'],
    queryFn: () => api.get('/api/audit/history?limit=20').then(r => r.data)
  })

  const trades = history ?? []

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <p className="mono text-xs text-text-muted tracking-widest">TRADE HISTORY</p>
        <p className="mono text-xs text-text-muted">{trades.length} recent entries</p>
      </div>

      <div className="overflow-x-auto">
        <table className="w-full text-sm min-w-[700px]">
          <thead>
            <tr className="border-b border-border">
              {['Timestamp', 'Asset', 'Side', 'Qty', 'Price', 'Value'].map((h) => (
                <th key={h} className="text-left px-5 py-3 mono text-xs text-text-muted tracking-wider">
                  {h}
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {isLoading && (
              <tr>
                <td colSpan="6" className="px-5 py-8 text-center text-text-muted mono text-xs skeleton">
                  Loading trade history...
                </td>
              </tr>
            )}
            {!isLoading && trades.length === 0 && (
              <tr>
                <td colSpan="6" className="px-5 py-8 text-center text-text-muted mono text-xs">
                  No trades found.
                </td>
              </tr>
            )}
            {!isLoading && trades.map((trade) => (
              <tr key={trade.id} className="border-b border-border hover:bg-bg/50 transition-colors">
                <td className="px-5 py-3 mono text-xs text-text-muted whitespace-nowrap">
                  {formatTime(trade.executed_at)}
                </td>
                <td className="px-5 py-3 mono text-xs text-info font-medium">
                  {trade.symbol}
                </td>
                <td className="px-5 py-3">
                  <span className={trade.entry_type === 'DEBIT' ? 'badge-profit' : 'badge-loss'}>
                    {trade.entry_type === 'DEBIT' ? 'BUY' : 'SELL'}
                  </span>
                </td>
                <td className="px-5 py-3 mono text-xs text-text-primary">{trade.qty}</td>
                <td className="px-5 py-3 mono text-xs text-text-primary">
                  ₹{Number(trade.executed_price).toLocaleString('en-IN', { minimumFractionDigits: 2 })}
                </td>
                <td className="px-5 py-3 mono text-xs text-text-primary font-medium">
                  ₹{Number(trade.total_value).toLocaleString('en-IN', { minimumFractionDigits: 2 })}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}
