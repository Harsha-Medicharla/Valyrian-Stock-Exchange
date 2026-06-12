import { useState } from 'react'
import { useQuery } from '@tanstack/react-query'
import api from '../api/axios'

function fmt(n) {
  return '₹' + n.toLocaleString('en-IN')
}

const COLUMNS = [
  { key: 'symbol',       label: 'Symbol'         },
  { key: 'name',         label: 'Name'            },
  { key: 'qty',          label: 'Qty'             },
  { key: 'avgCostBasis', label: 'Avg Cost'        },
  { key: 'currentPrice', label: 'Current Price'   },
  { key: 'marketValue',  label: 'Market Value'    },
  { key: 'unrealizedPnl',label: 'P&L'             },
  { key: 'pnlPct',       label: 'P&L %'           },
]

export default function HoldingsTable({ selectedAssetId, onSelectAsset }) {
  const { data, isLoading } = useQuery({
    queryKey: ['portfolio'],
    queryFn: () => api.get('/api/portfolio').then(r => r.data)
  })
  const holdings = data?.holdings ?? []

  const [sortKey, setSortKey] = useState('marketValue')
  const [sortDir, setSortDir] = useState('desc')

  function handleSort(key) {
    if (sortKey === key) {
      setSortDir((d) => (d === 'asc' ? 'desc' : 'asc'))
    } else {
      setSortKey(key)
      setSortDir('desc')
    }
  }

  const sorted = [...holdings].sort((a, b) => {
    const va = a[sortKey]
    const vb = b[sortKey]
    if (typeof va === 'string') return sortDir === 'asc' ? va.localeCompare(vb) : vb.localeCompare(va)
    return sortDir === 'asc' ? va - vb : vb - va
  })

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <p className="mono text-xs text-text-muted tracking-widest">HOLDINGS</p>
        <p className="mono text-xs text-text-muted">{holdings.length} positions</p>
      </div>
      <div className="overflow-x-auto">
        <table className="w-full text-sm min-w-[720px]">
          <thead>
            <tr className="border-b border-border">
              {COLUMNS.map((col) => (
                <th
                  key={col.key}
                  onClick={() => handleSort(col.key)}
                  className="text-left px-5 py-3 mono text-xs text-text-muted tracking-wider cursor-pointer hover:text-text-primary select-none whitespace-nowrap"
                >
                  {col.label}
                  {sortKey === col.key && (
                    <span className="ml-1 text-info">{sortDir === 'asc' ? '↑' : '↓'}</span>
                  )}
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {isLoading && (
              <tr>
                <td colSpan="8" className="px-5 py-6 text-center text-text-muted mono text-xs skeleton">
                  Loading holdings...
                </td>
              </tr>
            )}
            {!isLoading && sorted.map((h) => {
              const isSelected = h.assetId === selectedAssetId
              const pnlPos = h.unrealizedPnl >= 0
              return (
                <tr
                  key={h.assetId}
                  onClick={() => onSelectAsset && onSelectAsset(h.assetId)}
                  className={`border-b border-border cursor-pointer transition-colors hover:bg-surface ${
                    isSelected ? 'bg-[#13192a]' : ''
                  }`}
                  style={{ borderColor: isSelected ? '#1e3a5f' : undefined }}
                >
                  <td className="px-5 py-3 mono text-info font-medium">{h.symbol}</td>
                  <td className="px-5 py-3 text-text-primary text-xs">{h.name}</td>
                  <td className="px-5 py-3 mono text-text-primary">{h.qty}</td>
                  <td className="px-5 py-3 mono text-text-muted">{fmt(h.avgCostBasis)}</td>
                  <td className="px-5 py-3 mono text-text-primary">{fmt(h.currentPrice)}</td>
                  <td className="px-5 py-3 mono text-text-primary">{fmt(h.marketValue)}</td>
                  <td className="px-5 py-3">
                    <span className={pnlPos ? 'badge-profit' : 'badge-loss'}>
                      {pnlPos ? '+' : ''}{fmt(h.unrealizedPnl)}
                    </span>
                  </td>
                  <td className="px-5 py-3">
                    <span className={pnlPos ? 'badge-profit' : 'badge-loss'}>
                      {pnlPos ? '+' : ''}{h.pnlPct.toFixed(2)}%
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
