import { useState, useEffect } from 'react'
import { useAuth } from '../context/AuthContext'
import { getUserFunds, getUserHoldings, getUserHistory } from '../services/api'
import StatCard from '../components/StatCard'

export default function PortfolioPage() {
    const { userId } = useAuth()
    const [funds, setFunds] = useState(null)
    const [holdings, setHoldings] = useState([])
    const [history, setHistory] = useState([])
    const [activeTab, setActiveTab] = useState('holdings')
    const [loading, setLoading] = useState(true)

    useEffect(() =>{
        if (!userId) return
        Promise.all([
            getUserFunds(userId),
            getUserHoldings(userId),
            getUserHistory(userId),
        ]).then(([fundsRes, holdingsRes, historyRes]) =>{
            if (fundsRes.ok && fundsRes.data.status === 'success') setFunds(fundsRes.data.data)
            if (holdingsRes.ok && holdingsRes.data.status === 'success') setHoldings(holdingsRes.data.data.holdings || [])
            if (historyRes.ok && historyRes.data.status === 'success') setHistory(historyRes.data.data.history || [])
        }).finally(() =>setLoading(false))
    }, [userId])

    const fmt = (val) =>{
        if (val === null || val === undefined) return '—'
        const amount = val / 10000
        return '₹'+ amount.toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
    }

    if (loading) {
        return (
            <div className="page-container">
                <div className="loading-screen">
                    <div className="spinner" />
                    <span>Loading portfolio...</span>
                </div>
            </div>
        )
    }

    const cashBalance = funds?.cash_balance ?? 0
    const blockedFunds = funds?.blocked_funds ?? 0

    return (
        <div className="page-container">
            <div className="page-header">
                <h1>Portfolio</h1>
                <p>Your complete holdings, fund breakdown, and order history</p>
            </div>

            {/* Fund Stats */}
            <div className="grid-3" style={{ marginBottom: 'var(--space-xl)'}}>
                <StatCard
                    label="Cash Balance"
                    value={fmt(cashBalance)}
                    sub="Total funds in account"
                    color="var(--color-buy)"
                />
                <StatCard
                    label="Blocked Funds"
                    value={fmt(blockedFunds)}
                    sub="Locked in pending orders"
                    color="var(--color-warning)"
                />
                <StatCard
                    label="Available"
                    value={fmt(cashBalance - blockedFunds)}
                    sub="Ready for new orders"
                    color="var(--accent-primary)"
                />
            </div>

            {/* Tab Switcher */}
            <div style={{
                display: 'flex',
                gap: '4px',
                background: 'var(--bg-input)',
                borderRadius: 'var(--radius-md)',
                padding: '3px',
                marginBottom: 'var(--space-lg)',
                maxWidth: '400px',
            }}>
                {[
                    { key: 'holdings', label: 'Holdings', count: holdings.length },
                    { key: 'orders', label: 'Order History', count: history.length },
                ].map(tab =>(
                    <button
                        key={tab.key}
                        className={`btn ${activeTab === tab.key ? 'btn-primary': 'btn-ghost'}`}
                        style={{
                            flex: 1,
                            padding: '8px 16px',
                            border: activeTab === tab.key ? 'none': '1px solid transparent',
                        }}
                        onClick={() =>setActiveTab(tab.key)}
                   >
                        {tab.label} ({tab.count})
                    </button>
                ))}
            </div>

            {/* Holdings Tab */}
            {activeTab === 'holdings'&& (
                <div className="glass-card" style={{ padding: 'var(--space-lg)', overflow: 'auto'}}>
                    {holdings.length === 0 ? (
                        <div className="empty-state">
                            <div className="empty-icon"></div>
                            <p>No holdings yet. Buy some stocks to get started!</p>
                        </div>
                    ) : (
                        <table className="data-table">
                            <thead>
                                <tr>
                                    <th>Symbol</th>
                                    <th>Quantity</th>
                                    <th>Blocked Qty</th>
                                    <th>Available</th>
                                    <th>Status</th>
                                </tr>
                            </thead>
                            <tbody>
                                {holdings.map((h, i) =>(
                                    <tr key={i}>
                                        <td>
                                            <span className="text-mono font-bold text-accent">{h.symbol}</span>
                                        </td>
                                        <td className="text-mono font-semibold">{h.quantity?.toLocaleString()}</td>
                                        <td className="text-mono text-secondary">{h.blocked_qty?.toLocaleString()}</td>
                                        <td className="text-mono text-green">
                                            {(h.quantity - h.blocked_qty)?.toLocaleString()}
                                        </td>
                                        <td>
                                            <span className={`badge ${h.blocked_qty>0 ? 'badge-warning': 'badge-success'}`}>
                                                {h.blocked_qty>0 ? 'Partially Blocked': 'Free'}
                                            </span>
                                        </td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    )}
                </div>
            )}

            {/* Orders Tab */}
            {activeTab === 'orders'&& (
                <div className="glass-card" style={{ padding: 'var(--space-lg)', overflow: 'auto'}}>
                    {history.length === 0 ? (
                        <div className="empty-state">
                            <div className="empty-icon"></div>
                            <p>No orders placed yet</p>
                        </div>
                    ) : (
                        <table className="data-table">
                            <thead>
                                <tr>
                                    <th>Order ID</th>
                                    <th>Symbol</th>
                                    <th>Side</th>
                                    <th>Type</th>
                                    <th>Price</th>
                                    <th>Quantity</th>
                                    <th>Status</th>
                                    <th>Date</th>
                                </tr>
                            </thead>
                            <tbody>
                                {history.map((o, i) =>(
                                    <tr key={i}>
                                        <td className="text-mono text-secondary">#{o.order_id}</td>
                                        <td className="text-mono font-semibold">{o.symbol}</td>
                                        <td>
                                            <span className={`badge ${o.side === 'BUY'? 'badge-success': 'badge-danger'}`}>
                                                {o.side}
                                            </span>
                                        </td>
                                        <td>
                                            <span className="badge badge-info">{o.type}</span>
                                        </td>
                                        <td className="text-mono">₹{o.price?.toFixed(2)}</td>
                                        <td className="text-mono">{o.quantity?.toLocaleString()}</td>
                                        <td>
                                            <span className={`badge ${o.status === 'FILLED'? 'badge-success':
                                                o.status === 'REJECTED'|| o.status === 'CANCELLED'? 'badge-danger':
                                                    'badge-warning'
                                                }`}>
                                                {o.status}
                                            </span>
                                        </td>
                                        <td className="text-sm text-secondary">{o.created_at}</td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    )}
                </div>
            )}
        </div>
    )
}
