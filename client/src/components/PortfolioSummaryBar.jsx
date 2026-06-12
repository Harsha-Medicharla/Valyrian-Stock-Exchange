import { useQuery } from '@tanstack/react-query'
import StatCard from './StatCard'
import api from '../api/axios'

function fmt(n) {
  return '₹' + n.toLocaleString('en-IN')
}

export default function PortfolioSummaryBar() {
  const { data, isLoading } = useQuery({
    queryKey: ['portfolio'],
    queryFn: () => api.get('/api/portfolio').then(r => r.data)
  })

  if (isLoading || !data) {
    return (
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-3">
        {[1, 2, 3, 4].map(k => <div key={k} className="h-[88px] surface rounded-sm skeleton" />)}
      </div>
    )
  }

  const pnlColor = data.unrealizedPnl >= 0 ? 'profit' : 'loss'
  const dayColor = data.dailyChange >= 0 ? 'profit' : 'loss'

  return (
    <div className="grid grid-cols-2 lg:grid-cols-4 gap-3">
      <StatCard
        label="MTM Value"
        value={fmt(data.totalMtm)}
        subValue="mark-to-market"
      />
      <StatCard
        label="Cash Balance"
        value={fmt(data.cashBalance)}
        subValue="available to trade"
      />
      <StatCard
        label="Unrealized P&L"
        value={(data.unrealizedPnl >= 0 ? '+' : '') + fmt(data.unrealizedPnl)}
        subValue="open positions"
        color={pnlColor}
      />
      <StatCard
        label="Daily Change"
        value={(data.dailyChange >= 0 ? '+' : '') + data.dailyChange.toFixed(2) + '%'}
        subValue="vs yesterday's close"
        color={dayColor}
      />
    </div>
  )
}
