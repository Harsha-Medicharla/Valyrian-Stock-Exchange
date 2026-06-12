import {
  LineChart, Line, XAxis, YAxis, CartesianGrid,
  Tooltip, ResponsiveContainer,
} from 'recharts'
import { useQuery } from '@tanstack/react-query'
import api from '../api/axios'

function fmt(n) {
  return '₹' + Number(n).toLocaleString('en-IN')
}

function CustomTooltip({ active, payload, label }) {
  if (!active || !payload?.length) return null
  return (
    <div className="bg-surface border border-border px-3 py-2 rounded-sm">
      <p className="mono text-xs text-text-muted">{label}</p>
      <p className="mono text-sm text-info">{fmt(payload[0].value)}</p>
    </div>
  )
}

export default function PriceChart({ selectedAssetId, assets = [] }) {
  const assetId = selectedAssetId ?? assets[0]?.assetId ?? null
  
  const { data: historyData, isLoading: isLoadingHistory } = useQuery({
    queryKey: ['history', assetId],
    queryFn: () => api.get(`/api/portfolio/history?assetId=${assetId}&range=14d`).then(r => r.data),
    enabled: assetId != null
  })

  if (assetId == null) {
    return (
      <div className="surface rounded-sm overflow-hidden h-[290px] flex items-center justify-center">
        <span className="mono text-xs text-text-muted">No asset selected.</span>
      </div>
    )
  }

  if (isLoadingHistory || !historyData) {
    return (
      <div className="surface rounded-sm overflow-hidden h-[290px] flex items-center justify-center skeleton">
        <span className="mono text-xs text-text-muted">Loading chart...</span>
      </div>
    )
  }

  const asset = assets.find((item) => item.assetId === assetId)

  const prices = historyData.prices
  const first = prices[0]?.price || 0
  const last = prices[prices.length - 1]?.price || 0
  const change = last - first
  const changePct = ((change / first) * 100).toFixed(2)
  const isUp = change >= 0

  return (
    <div className="surface rounded-sm overflow-hidden">
      {/* Header */}
      <div className="px-5 py-4 border-b border-border flex items-start justify-between">
        <div>
          <div className="flex items-center gap-3">
            <span className="mono text-info font-medium">{historyData.symbol}</span>
            <span className="text-xs text-text-muted">{historyData.name || asset?.name}</span>
          </div>
          <p className="mono text-xl font-medium text-text-primary mt-1">{fmt(last)}</p>
        </div>
        <div className="text-right">
          <span className={`badge-${isUp ? 'profit' : 'loss'} text-xs`}>
            {isUp ? '+' : ''}{changePct}%
          </span>
          <p className="mono text-xs text-text-muted mt-1">14-day range</p>
        </div>
      </div>

      {/* Chart */}
      <div className="p-4 h-52">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={prices} margin={{ top: 4, right: 8, bottom: 0, left: 0 }}>
            <CartesianGrid stroke="#1f1f1f" strokeDasharray="3 3" vertical={false} />
            <XAxis
              dataKey="date"
              tick={{ fill: '#6b6b6b', fontSize: 10, fontFamily: 'IBM Plex Mono' }}
              tickLine={false}
              axisLine={false}
              tickFormatter={(d) => d.slice(5)} // show MM-DD
            />
            <YAxis
              tick={{ fill: '#6b6b6b', fontSize: 10, fontFamily: 'IBM Plex Mono' }}
              tickLine={false}
              axisLine={false}
              tickFormatter={(v) => '₹' + v.toLocaleString('en-IN')}
              width={72}
            />
            <Tooltip content={<CustomTooltip />} cursor={{ stroke: '#1f1f1f', strokeWidth: 1 }} />
            <Line
              type="monotone"
              dataKey="price"
              stroke={isUp ? '#22c55e' : '#ef4444'}
              strokeWidth={1.5}
              dot={false}
              activeDot={{ r: 3, fill: isUp ? '#22c55e' : '#ef4444' }}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  )
}
