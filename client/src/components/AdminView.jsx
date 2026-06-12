import { useState } from 'react'
import { useQuery, useQueryClient } from '@tanstack/react-query'
import api from '../api/axios'
import StatCard from './StatCard'
import AuditFeed from './AuditFeed'

function fmt(n) {
  return Number(n || 0).toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

function UserStates({ users = [] }) {
  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <p className="mono text-xs text-text-muted tracking-widest">USER ACCOUNTS</p>
        <p className="mono text-[10px] text-text-muted">{users.length} registered</p>
      </div>

      {users.length === 0 ? (
        <div className="px-5 py-6">
          <p className="mono text-xs text-text-muted text-center">No users found.</p>
        </div>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-sm min-w-[640px]">
            <thead>
              <tr className="border-b border-border">
                {['ID', 'Name', 'Email', 'Role', 'Wallet', 'Created'].map((h) => (
                  <th key={h} className="text-left px-5 py-3 mono text-[10px] text-text-muted tracking-wider">
                    {h}
                  </th>
                ))}
              </tr>
            </thead>
            <tbody>
              {users.map((user) => (
                <tr key={user.id} className="border-b border-border hover:bg-bg/40">
                  <td className="px-5 py-2 mono text-xs text-text-muted">{user.id}</td>
                  <td className="px-5 py-2 mono text-xs text-text-primary">{user.name}</td>
                  <td className="px-5 py-2 mono text-xs text-info">{user.email}</td>
                  <td className="px-5 py-2">
                    <span className={user.role === 'ADMIN' ? 'badge-warn' : 'badge-profit'}>
                      {user.role}
                    </span>
                  </td>
                  <td className="px-5 py-2 mono text-xs text-text-primary">₹{fmt(user.balance)}</td>
                  <td className="px-5 py-2 mono text-[10px] text-text-muted whitespace-nowrap">
                    {new Date(user.created_at).toLocaleString('en-IN', { hour12: false })}
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

// ─── Funds Manager ──────────────────────────────────────────────────────────
function FundsManager() {
  const [userId, setUserId] = useState('')
  const [amount, setAmount] = useState('')
  const [loading, setLoading] = useState(null) // 'deposit' | 'withdraw' | null
  const [msg, setMsg] = useState(null) // { type: 'ok'|'err', text }

  async function handleAction(action) {
    const uid = parseInt(userId, 10)
    const amt = parseFloat(amount)
    if (!uid || isNaN(amt) || amt <= 0) {
      setMsg({ type: 'err', text: 'Enter a valid User ID and positive amount.' })
      return
    }
    setLoading(action)
    setMsg(null)
    try {
      const res = await api.post(`/api/admin/${action}`, { userId: uid, amount: amt })
      setMsg({
        type: 'ok',
        text: `${action.toUpperCase()} successful. New balance: ₹${fmt(res.data.newBalance)}`
      })
      setUserId('')
      setAmount('')
    } catch (err) {
      setMsg({ type: 'err', text: err.response?.data?.error || `${action} failed.` })
    } finally {
      setLoading(null)
    }
  }

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">FUND MANAGEMENT</p>
        <p className="mono text-[10px] text-text-muted mt-0.5">
          Calls deposit_funds / withdraw_funds stored procedures atomically
        </p>
      </div>

      <div className="p-5 space-y-4">
        <div className="grid grid-cols-2 gap-4">
          <div>
            <label className="block mono text-[10px] text-text-muted mb-1.5 tracking-wider">USER ID</label>
            <input
              type="number"
              min="1"
              value={userId}
              onChange={e => setUserId(e.target.value)}
              placeholder="e.g. 1"
              className="w-full px-3 py-2 text-sm rounded-sm mono"
            />
          </div>
          <div>
            <label className="block mono text-[10px] text-text-muted mb-1.5 tracking-wider">AMOUNT (₹)</label>
            <input
              type="number"
              min="0.01"
              step="0.01"
              value={amount}
              onChange={e => setAmount(e.target.value)}
              placeholder="e.g. 10000"
              className="w-full px-3 py-2 text-sm rounded-sm mono"
            />
          </div>
        </div>

        <div className="grid grid-cols-2 gap-3">
          <button
            onClick={() => handleAction('deposit')}
            disabled={!!loading}
            className="mono text-xs font-medium py-2.5 tracking-widest rounded-sm border border-profit/40 text-profit bg-profit/10 hover:bg-profit/20 transition-colors disabled:opacity-40 disabled:cursor-not-allowed"
          >
            {loading === 'deposit' ? 'DEPOSITING...' : '↑ DEPOSIT'}
          </button>
          <button
            onClick={() => handleAction('withdraw')}
            disabled={!!loading}
            className="mono text-xs font-medium py-2.5 tracking-widest rounded-sm border border-loss/40 text-loss bg-loss/10 hover:bg-loss/20 transition-colors disabled:opacity-40 disabled:cursor-not-allowed"
          >
            {loading === 'withdraw' ? 'WITHDRAWING...' : '↓ WITHDRAW'}
          </button>
        </div>

        {msg && (
          <div className={`px-3 py-2 rounded-sm border ${msg.type === 'ok'
            ? 'bg-profit/10 border-profit/30'
            : 'bg-loss/10 border-loss/30'}`}>
            <p className={`mono text-xs ${msg.type === 'ok' ? 'text-profit' : 'text-loss'}`}>
              {msg.text}
            </p>
          </div>
        )}

        <p className="mono text-[10px] text-text-muted">
          These operations call the <span className="text-info">deposit_funds</span> /{' '}
          <span className="text-info">withdraw_funds</span> stored procedures, which use{' '}
          <span className="text-info">SELECT FOR UPDATE</span> and insert into{' '}
          <span className="text-info">ledger_entries</span> atomically.
        </p>
      </div>
    </div>
  )
}

// ─── Running Balance Timeline ────────────────────────────────────────────────
function RunningBalance({ allUsers = [] }) {
  const [selectedUserId, setSelectedUserId] = useState(1)

  const { data: balanceHistory = [], isLoading } = useQuery({
    queryKey: ['admin-running-balance', selectedUserId],
    queryFn: () =>
      api.get(`/api/admin/balance-history/${selectedUserId}`).then(r => r.data),
    staleTime: 15_000,
  })

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
        <div>
          <p className="mono text-xs text-text-muted tracking-widest">RUNNING WALLET BALANCE</p>
          <p className="mono text-[10px] text-text-muted mt-0.5">
            running_balance_view · SUM() OVER(PARTITION BY user_id ORDER BY created_at)
          </p>
        </div>
        <select
          value={selectedUserId}
          onChange={e => setSelectedUserId(Number(e.target.value))}
          className="mono text-xs px-2 py-1.5 rounded-sm border border-border bg-bg text-text-primary"
        >
          {allUsers.length > 0
            ? allUsers.map(u => <option key={u.id} value={u.id}>{u.name}</option>)
            : [1,2,3,4].map(id => <option key={id} value={id}>User {id}</option>)
          }
        </select>
      </div>

      {isLoading ? (
        <div className="p-5 space-y-2 animate-pulse">
          {[...Array(4)].map((_, i) => <div key={i} className="h-8 bg-border rounded-sm" />)}
        </div>
      ) : balanceHistory.length === 0 ? (
        <p className="mono text-xs text-text-muted px-5 py-6 text-center">
          No ledger entries found for this user.
        </p>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-sm min-w-[540px]">
            <thead>
              <tr className="border-b border-border">
                {['Ledger ID', 'Trade', 'Type', 'Amount', 'Running Balance', 'Time'].map(h => (
                  <th key={h} className="text-left px-5 py-3 mono text-[10px] text-text-muted tracking-wider">{h}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {balanceHistory.slice(-15).reverse().map(row => {
                const isCredit = row.entry_type === 'CREDIT'
                return (
                  <tr key={row.ledger_id} className="border-b border-border hover:bg-bg/40">
                    <td className="px-5 py-2 mono text-xs text-text-muted">{row.ledger_id}</td>
                    <td className="px-5 py-2 mono text-xs text-text-muted">
                      {row.trade_id ? `TX_${row.trade_id}` : <span className="text-warn">MANUAL</span>}
                    </td>
                    <td className="px-5 py-2">
                      <span className={isCredit ? 'badge-profit' : 'badge-loss'}>{row.entry_type}</span>
                    </td>
                    <td className={`px-5 py-2 mono text-xs font-medium ${isCredit ? 'text-profit' : 'text-loss'}`}>
                      {isCredit ? '+' : '-'}₹{fmt(row.amount)}
                    </td>
                    <td className="px-5 py-2 mono text-xs text-text-primary font-medium">
                      ₹{fmt(row.running_balance)}
                    </td>
                    <td className="px-5 py-2 mono text-[10px] text-text-muted whitespace-nowrap">
                      {new Date(row.created_at).toLocaleString('en-IN', { hour12: false })}
                    </td>
                  </tr>
                )
              })}
            </tbody>
          </table>
        </div>
      )}
    </div>
  )
}

// ─── Price Manager — fires price_history_trigger live ────────────────────────
function PriceManager({ assets = [] }) {
  const queryClient = useQueryClient()
  const [assetId, setAssetId] = useState('')
  const [price, setPrice] = useState('')
  const [loading, setLoading] = useState(false)
  const [msg, setMsg] = useState(null)

  async function handleUpdate() {
    const id = parseInt(assetId, 10)
    const p = parseFloat(price)
    if (!id || isNaN(p) || p <= 0) {
      setMsg({ type: 'err', text: 'Select an asset and enter a positive price.' })
      return
    }
    setLoading(true)
    setMsg(null)
    try {
      const res = await api.patch(`/api/admin/assets/${id}/price`, { price: p })
      const a = res.data.asset
      setMsg({
        type: 'ok',
        text: `${a.symbol} updated to ₹${Number(a.current_price).toLocaleString('en-IN')} — price_history_trigger fired ✓`
      })
      setPrice('')
      // Invalidate so price chart + portfolio refresh
      queryClient.invalidateQueries({ queryKey: ['assets'] })
      queryClient.invalidateQueries({ queryKey: ['portfolio'] })
      queryClient.invalidateQueries({ queryKey: ['history'] })
    } catch (err) {
      setMsg({ type: 'err', text: err.response?.data?.error || 'Update failed.' })
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="surface rounded-sm overflow-hidden">
      <div className="px-5 py-3 border-b border-border">
        <p className="mono text-xs text-text-muted tracking-widest">ASSET PRICE UPDATER</p>
        <p className="mono text-[10px] text-text-muted mt-0.5">
          Updates <span className="text-info">assets.current_price</span> → fires{' '}
          <span className="text-info">price_history_trigger</span> → inserts into{' '}
          <span className="text-info">price_history</span> automatically
        </p>
      </div>

      <div className="p-5 space-y-4">
        <div className="grid grid-cols-2 gap-4">
          <div>
            <label className="block mono text-[10px] text-text-muted mb-1.5 tracking-wider">ASSET</label>
            <select
              value={assetId}
              onChange={e => setAssetId(e.target.value)}
              className="w-full px-3 py-2 text-sm rounded-sm mono border border-border"
            >
              <option value="">Select asset...</option>
              {assets.map(a => (
                <option key={a.assetId ?? a.id} value={a.assetId ?? a.id}>
                  {a.symbol} — ₹{Number(a.currentPrice ?? a.current_price).toLocaleString('en-IN')}
                </option>
              ))}
            </select>
          </div>
          <div>
            <label className="block mono text-[10px] text-text-muted mb-1.5 tracking-wider">NEW PRICE (₹)</label>
            <input
              type="number"
              min="0.01"
              step="0.01"
              value={price}
              onChange={e => setPrice(e.target.value)}
              placeholder="e.g. 2650.00"
              className="w-full px-3 py-2 text-sm rounded-sm mono"
            />
          </div>
        </div>

        <button
          onClick={handleUpdate}
          disabled={loading}
          className="w-full mono text-xs font-medium py-2.5 tracking-widest rounded-sm border border-info/40 text-info bg-info/10 hover:bg-info/20 transition-colors disabled:opacity-40 disabled:cursor-not-allowed"
        >
          {loading ? 'UPDATING...' : '↻ UPDATE PRICE (FIRE TRIGGER)'}
        </button>

        {msg && (
          <div className={`px-3 py-2 rounded-sm border ${msg.type === 'ok' ? 'bg-profit/10 border-profit/30' : 'bg-loss/10 border-loss/30'}`}>
            <p className={`mono text-xs ${msg.type === 'ok' ? 'text-profit' : 'text-loss'}`}>{msg.text}</p>
          </div>
        )}
      </div>
    </div>
  )
}

// ─── Main AdminView ──────────────────────────────────────────────────────────
export default function AdminView() {
  const statsQuery = useQuery({
    queryKey: ['admin-stats'],
    queryFn: () => api.get('/api/admin/stats').then(r => r.data),
    refetchInterval: 3000
  })

  const locksQuery = useQuery({
    queryKey: ['admin-locks'],
    queryFn: () => api.get('/api/admin/locks').then(r => r.data),
    refetchInterval: 2000
  })

  // Load assets for the price updater
  const { data: assets = [] } = useQuery({
    queryKey: ['assets'],
    queryFn: () => api.get('/api/assets').then(r => r.data),
    staleTime: 60_000,
  })

  const { data: allUsers = [] } = useQuery({
    queryKey: ['admin-users'],
    queryFn: () => api.get('/api/admin/users').then(r => r.data),
    staleTime: 60_000,
  })

  const stats = statsQuery.data
  const locks = locksQuery.data ?? []
  const isLoading = statsQuery.isLoading || locksQuery.isLoading


  if (isLoading && !stats) return (
    <div className="surface rounded-sm p-8 flex items-center justify-center animate-pulse">
      <span className="mono text-xs text-text-muted">LOADING SYSTEM MONITOR...</span>
    </div>
  )

  return (
    <div className="space-y-6">
      {/* System Health */}
      <div>
        <p className="mono text-xs text-text-muted tracking-widest mb-3">SYSTEM INFRASTRUCTURE</p>
        <div className="grid grid-cols-2 lg:grid-cols-4 gap-3">
          <StatCard
            label="Service Status"
            value={stats?.status || 'OFFLINE'}
            subValue="express server"
            color={stats?.status === 'HEALTHY' ? 'profit' : 'loss'}
          />
          <StatCard
            label="App Uptime"
            value={`${Math.floor((stats?.uptime || 0) / 60)}m ${Math.floor((stats?.uptime || 0) % 60)}s`}
            subValue="active session"
          />
          <StatCard
            label="Active Pool"
            value={stats?.connections == null ? 'N/A' : stats.connections.toString()}
            subValue="DB connections"
            color={(stats?.connections || 0) > 5 ? 'warn' : 'default'}
          />
          <StatCard
            label="Idle Pool"
            value={stats?.idleConnections == null ? 'N/A' : stats.idleConnections.toString()}
            subValue="available seats"
          />
        </div>
      </div>

      <UserStates users={allUsers} />

      {/* Locks + Audit side-by-side */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Active Locks */}
        <div className="surface rounded-sm overflow-hidden">
          <div className="px-5 py-3 border-b border-border flex items-center justify-between">
            <p className="mono text-xs text-text-muted tracking-widest">DATABASE LOCKS</p>
            {locks.length > 0 && (
              <span className="badge-warn">{locks.length} ACTIVE</span>
            )}
          </div>

          {locks.length === 0 ? (
            <div className="px-5 py-8 text-center">
              <p className="mono text-xs text-text-muted tracking-widest">NO ACTIVE INNODB TRANSACTIONS</p>
            </div>
          ) : (
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-border">
                  {['TRX ID', 'State', 'Started', 'Query'].map((h) => (
                    <th key={h} className="text-left px-5 py-3 mono text-xs text-text-muted tracking-wider">
                      {h}
                    </th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {locks.map((lock, i) => (
                  <tr key={i} className="border-b border-border">
                    <td className="px-5 py-3 mono text-xs text-text-muted font-medium">
                      {lock.trx_id}
                    </td>
                    <td className="px-5 py-3 mono text-xs">
                      <span className="badge-warn">{lock.trx_state}</span>
                    </td>
                    <td className="px-5 py-3 mono text-xs text-text-muted">
                      {new Date(lock.trx_started).toLocaleTimeString()}
                    </td>
                    <td className="px-5 py-3 mono text-xs text-info truncate max-w-[150px]" title={lock.trx_query}>
                      {lock.trx_query || 'COMMIT/WAITING'}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>

        {/* Audit Feed */}
        <div className="h-80">
          <AuditFeed />
        </div>
      </div>

      {/* Fund Management + Running Balance */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <FundsManager />
        <RunningBalance allUsers={allUsers} />
      </div>

      {/* Price Manager — fires price_history_trigger live */}
      <PriceManager assets={assets} />
    </div>
  )
}
