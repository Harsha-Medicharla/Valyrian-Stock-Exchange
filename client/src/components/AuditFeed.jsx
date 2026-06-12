import { useRef, useEffect } from 'react'
import { useQuery } from '@tanstack/react-query'
import api from '../api/axios'

function truncateHash(hash) {
  if (!hash) return ''
  return hash.slice(0, 16) + '...' + hash.slice(-8)
}

export default function AuditFeed() {
  const { data: logs, isLoading } = useQuery({
    queryKey: ['audit'],
    queryFn: () => api.get('/api/audit?limit=20').then(r => r.data),
    refetchInterval: 5000
  })

  const auditLogs = logs?.entries ?? []
  const feedRef = useRef(null)

  // Auto-scroll to top on new logs
  useEffect(() => {
    if (feedRef.current) {
      feedRef.current.scrollTop = 0
    }
  }, [auditLogs.length])

  return (
    <div className="surface rounded-sm h-full flex flex-col overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <p className="mono text-xs text-text-muted tracking-widest">LIVE AUDIT FEED</p>
        <div className="flex items-center gap-2">
          <div className="w-1.5 h-1.5 rounded-full bg-profit animate-pulse" />
          <span className="mono text-[10px] text-profit tracking-widest">LIVE</span>
        </div>
      </div>

      <div
        ref={feedRef}
        className="flex-1 overflow-y-auto p-5 space-y-4 custom-scrollbar"
      >
        {isLoading && auditLogs.length === 0 && (
          <div className="mono text-xs text-text-muted animate-pulse">CONNECTING TO LEDGER...</div>
        )}

        {auditLogs.length === 0 && !isLoading && (
          <div className="mono text-xs text-text-muted">WAITING FOR NETWORK ACTIVITY...</div>
        )}

        {auditLogs.map((log) => (
          <div key={log.id} className="space-y-1.5 border-l border-info/30 pl-3 py-0.5">
            <div className="flex items-center justify-between">
              <span className="mono text-[10px] text-text-muted">
                {new Date(log.timestamp).toLocaleTimeString()}
              </span>
              <span className="mono text-[10px] text-info font-medium tracking-tighter">
                TX_{log.tradeId}
              </span>
            </div>
            <p className="mono text-xs text-text-primary break-all leading-relaxed">
              <span className="text-info font-bold">COMMIT</span> hash:{' '}
              <span className="text-text-muted">{truncateHash(log.txHash)}</span>
            </p>
            <div className="flex gap-2">
              <span className="mono text-[10px] bg-surface-lighter px-1.5 py-0.5 rounded-sm border border-border">
                {log.assetSymbol}
              </span>
              <span className="mono text-[10px] text-text-muted py-0.5">
                {log.qty} shares @ ₹{log.price}
              </span>
            </div>
          </div>
        ))}
      </div>
    </div>
  )
}
