import { useQuery } from '@tanstack/react-query'
import api from '../api/axios'

function fmt(n, decimals = 2) {
  return Number(n || 0).toLocaleString('en-IN', {
    minimumFractionDigits: decimals,
    maximumFractionDigits: decimals,
  })
}

function SectionHeader({ label }) {
  return (
    <p className="mono text-xs text-text-muted tracking-widest mb-3">{label}</p>
  )
}

// ─── User P&L Rankings ────────────────────────────────────────────────────────
function PnlSummaryTable({ data = [] }) {
  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">USER P&L RANKINGS</p>
        <p className="mono text-[10px] text-text-muted mt-0.5">user_pnl_summary_view · RANK() OVER(ORDER BY portfolio_value DESC)</p>
      </div>
      <div className="overflow-x-auto">
        <table className="w-full text-sm">
          <thead>
            <tr className="border-b border-border">
              {['Rank', 'User', 'Market Value', 'Cost Basis', 'Unrealised P&L', 'P&L %'].map(h => (
                <th key={h} className="text-left px-5 py-3 mono text-[10px] text-text-muted tracking-wider">{h}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {data.length === 0 && (
              <tr><td colSpan="6" className="px-5 py-6 mono text-xs text-text-muted text-center">No data</td></tr>
            )}
            {data.map(row => {
              const pnl = Number(row.unrealized_pnl || 0)
              const pnlPct = Number(row.pnl_pct || 0)
              return (
                <tr key={row.user_id} className="border-b border-border hover:bg-bg/40">
                  <td className="px-5 py-3 mono text-sm font-bold text-warn">#{row.portfolio_rank}</td>
                  <td className="px-5 py-3 mono text-xs text-text-primary font-medium">{row.user_name}</td>
                  <td className="px-5 py-3 mono text-xs text-text-primary">₹{fmt(row.total_market_value)}</td>
                  <td className="px-5 py-3 mono text-xs text-text-muted">₹{fmt(row.total_book_value)}</td>
                  <td className={`px-5 py-3 mono text-xs font-medium ${pnl >= 0 ? 'text-profit' : 'text-loss'}`}>
                    {pnl >= 0 ? '+' : ''}₹{fmt(pnl)}
                  </td>
                  <td className={`px-5 py-3`}>
                    <span className={pnlPct >= 0 ? 'badge-profit' : 'badge-loss'}>
                      {pnlPct >= 0 ? '+' : ''}{fmt(pnlPct, 2)}%
                    </span>
                  </td>
                </tr>
              )
            })}
          </tbody>
        </table>
      </div>
    </div>
  )
}

// ─── Asset Volume ─────────────────────────────────────────────────────────────
function AssetVolumeTable({ data = [] }) {
  const maxVol = Math.max(...data.map(r => Number(r.total_value_traded || 0)), 1)
  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">ASSET TRADING VOLUME</p>
        <p className="mono text-[10px] text-text-muted mt-0.5">asset_volume_view · GROUP BY aggregation on trades</p>
      </div>
      <div className="overflow-x-auto">
        <table className="w-full text-sm">
          <thead>
            <tr className="border-b border-border">
              {['Asset', 'Trade Count', 'Qty Traded', 'Total Value', 'Volume Bar'].map(h => (
                <th key={h} className="text-left px-5 py-3 mono text-[10px] text-text-muted tracking-wider">{h}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {data.length === 0 && (
              <tr><td colSpan="5" className="px-5 py-6 mono text-xs text-text-muted text-center">No data</td></tr>
            )}
            {data.map(row => {
              const pct = (Number(row.total_value_traded || 0) / maxVol) * 100
              return (
                <tr key={row.asset_id} className="border-b border-border hover:bg-bg/40">
                  <td className="px-5 py-3 mono text-xs text-info font-medium">{row.symbol}</td>
                  <td className="px-5 py-3 mono text-xs text-text-muted">{row.total_trades}</td>
                  <td className="px-5 py-3 mono text-xs text-text-primary">{fmt(row.total_quantity_traded, 0)}</td>
                  <td className="px-5 py-3 mono text-xs text-text-primary">₹{fmt(row.total_value_traded)}</td>
                  <td className="px-5 py-3 w-40">
                    <div className="h-1.5 bg-border rounded-full overflow-hidden">
                      <div className="h-full bg-info rounded-full" style={{ width: `${pct}%` }} />
                    </div>
                  </td>
                </tr>
              )
            })}
          </tbody>
        </table>
      </div>
    </div>
  )
}

// ─── User Trade Summary (correlated subquery view) ────────────────────────────
function TradeSummaryTable({ data = [] }) {
  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">USER TRADE STATISTICS</p>
        <p className="mono text-[10px] text-text-muted mt-0.5">user_trade_summary_view · correlated subquery for most-traded asset</p>
      </div>
      <div className="overflow-x-auto">
        <table className="w-full text-sm">
          <thead>
            <tr className="border-b border-border">
              {['User', 'Total Trades', 'Total Volume', 'Top Asset'].map(h => (
                <th key={h} className="text-left px-5 py-3 mono text-[10px] text-text-muted tracking-wider">{h}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {data.length === 0 && (
              <tr><td colSpan="4" className="px-5 py-6 mono text-xs text-text-muted text-center">No data</td></tr>
            )}
            {data.map(row => (
              <tr key={row.user_id} className="border-b border-border hover:bg-bg/40">
                <td className="px-5 py-3 mono text-xs text-text-primary font-medium">{row.user_name}</td>
                <td className="px-5 py-3 mono text-xs text-text-primary">{row.total_trades}</td>
                <td className="px-5 py-3 mono text-xs text-text-primary">₹{fmt(row.total_volume_traded)}</td>
                <td className="px-5 py-3">
                  {row.most_traded_asset ? (
                    <span className="mono text-xs text-info font-medium">{row.most_traded_asset}</span>
                  ) : (
                    <span className="mono text-xs text-text-muted">—</span>
                  )}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}

// ─── Exchange Capital Distribution (unpartitioned SUM() OVER()) ───────────────
function ExchangeWeightTable({ data = [] }) {
  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">EXCHANGE CAPITAL DISTRIBUTION</p>
        <p className="mono text-[10px] text-text-muted mt-0.5">user_exchange_weight_view · unpartitioned SUM() OVER() window function</p>
      </div>
      <div className="p-5 space-y-3">
        {data.length === 0 && (
          <p className="mono text-xs text-text-muted text-center">No data</p>
        )}
        {data.map(row => {
          const pct = Number(row.exchange_weight_pct || 0)
          return (
            <div key={row.user_id}>
              <div className="flex justify-between mb-1">
                <span className="mono text-xs text-text-primary">{row.user_name}</span>
                <span className="mono text-xs text-text-muted">
                  ₹{fmt(row.portfolio_value)} · <span className="text-info">{fmt(pct, 1)}%</span>
                </span>
              </div>
              <div className="h-2 bg-border rounded-full overflow-hidden">
                <div
                  className="h-full rounded-full transition-all"
                  style={{
                    width: `${Math.min(pct, 100)}%`,
                    background: `hsl(${220 + pct * 0.8}, 80%, 55%)`,
                  }}
                />
              </div>
            </div>
          )
        })}
      </div>
    </div>
  )
}

// ─── Open Orders (global view) ────────────────────────────────────────────────
function OpenOrdersTable({ data = [] }) {
  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <p className="mono text-xs text-text-muted tracking-widest">ALL OPEN ORDERS</p>
        <p className="mono text-xs text-text-muted">open_orders_view</p>
      </div>
      {data.length === 0 ? (
        <p className="mono text-xs text-text-muted px-5 py-6 text-center">NO OPEN ORDERS ON EXCHANGE</p>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-sm min-w-[600px]">
            <thead>
              <tr className="border-b border-border">
                {['#', 'Asset', 'Side', 'Type', 'Qty', 'Filled', 'Limit Price', 'Placed'].map(h => (
                  <th key={h} className="text-left px-4 py-3 mono text-[10px] text-text-muted tracking-wider">{h}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {data.slice(0, 20).map(o => (
                <tr key={o.order_id} className="border-b border-border">
                  <td className="px-4 py-2 mono text-xs text-text-muted">{o.order_id}</td>
                  <td className="px-4 py-2 mono text-xs text-info font-medium">{o.asset_symbol}</td>
                  <td className="px-4 py-2">
                    <span className={o.side === 'BUY' ? 'badge-profit' : 'badge-loss'}>{o.side}</span>
                  </td>
                  <td className="px-4 py-2 mono text-xs text-text-muted">{o.order_type}</td>
                  <td className="px-4 py-2 mono text-xs text-text-primary">{Number(o.qty).toFixed(0)}</td>
                  <td className="px-4 py-2 mono text-xs text-text-muted">
                    {Number(o.filled_qty || 0).toFixed(0)} / {Number(o.qty).toFixed(0)}
                  </td>
                  <td className="px-4 py-2 mono text-xs text-text-primary">
                    {o.limit_price ? `₹${fmt(o.limit_price)}` : <span className="text-text-muted">—</span>}
                  </td>
                  <td className="px-4 py-2 mono text-[10px] text-text-muted whitespace-nowrap">
                    {new Date(o.created_at).toLocaleString('en-IN', { hour12: false })}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  )
}

// ─── WAC View ─────────────────────────────────────────────────────────────────
function WacTable({ data = [] }) {
  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">WEIGHTED AVERAGE COST (WAC)</p>
        <p className="mono text-[10px] text-text-muted mt-0.5">wac_view · avg_cost_basis per user per holding</p>
      </div>
      {data.length === 0 ? (
        <p className="mono text-xs text-text-muted px-5 py-6 text-center">No holdings data</p>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-border">
                {['User', 'Asset', 'Qty Held', 'WAC (Avg Cost)', 'Total Invested'].map(h => (
                  <th key={h} className="text-left px-5 py-3 mono text-[10px] text-text-muted tracking-wider">{h}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {data.map((row, i) => (
                <tr key={`${row.user_id}-${row.asset_id ?? i}`} className="border-b border-border hover:bg-bg/40">
                  <td className="px-5 py-2 mono text-xs text-text-primary">{row.user_name}</td>
                  <td className="px-5 py-2 mono text-xs text-info font-medium">{row.symbol}</td>
                  <td className="px-5 py-2 mono text-xs text-text-primary">{fmt(row.quantity, 0)}</td>
                  <td className="px-5 py-2 mono text-xs text-text-primary">₹{fmt(row.avg_cost_basis)}</td>
                  <td className="px-5 py-2 mono text-xs text-text-muted">₹{fmt(row.total_invested)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  )
}

// ─── Main AnalyticsView ───────────────────────────────────────────────────────
export default function AnalyticsView() {
  const { data: analytics, isLoading } = useQuery({
    queryKey: ['analytics'],
    queryFn: () => api.get('/api/portfolio/analytics').then(r => r.data),
    staleTime: 30_000,
    refetchInterval: 30_000,
  })

  if (isLoading && !analytics) {
    return (
      <div className="space-y-4 animate-pulse">
        {[...Array(4)].map((_, i) => (
          <div key={i} className="surface rounded-sm p-8">
            <div className="h-4 bg-border rounded-sm w-48 mb-4" />
            <div className="space-y-2">
              {[...Array(3)].map((_, j) => <div key={j} className="h-8 bg-border rounded-sm" />)}
            </div>
          </div>
        ))}
      </div>
    )
  }

  return (
    <div className="space-y-6">
      <div>
        <p className="mono text-xs text-text-muted tracking-widest mb-1">DATABASE ANALYTICS</p>
        <p className="mono text-[10px] text-text-muted">
          Live data from 6 SQL views — window functions, correlated subqueries, GROUP BY aggregations, WAC calculation
        </p>
      </div>

      <div className="grid grid-cols-1 xl:grid-cols-2 gap-6">
        <PnlSummaryTable data={analytics?.pnlSummary ?? []} />
        <ExchangeWeightTable data={analytics?.exchangeWeight ?? []} />
      </div>

      <AssetVolumeTable data={analytics?.assetVolume ?? []} />

      <div className="grid grid-cols-1 xl:grid-cols-2 gap-6">
        <TradeSummaryTable data={analytics?.tradeSummary ?? []} />
        <OpenOrdersTable data={analytics?.openOrders ?? []} />
      </div>

      <WacTable data={analytics?.wacData ?? []} />
    </div>
  )
}
