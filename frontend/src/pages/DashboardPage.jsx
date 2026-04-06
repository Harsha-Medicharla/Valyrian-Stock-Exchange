import { useState, useEffect } from 'react'
import { useAuth } from '../context/AuthContext'
import { getUserFunds, getUserHoldings, getUserHistory } from '../services/api'
import StatCard from '../components/StatCard'
import { Link } from 'react-router-dom'

export default function DashboardPage() {
    const { userId, user } = useAuth()
    const [funds, setFunds] = useState(null)
    const [holdings, setHoldings] = useState([])
    const [history, setHistory] = useState([])
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
                    <span>Loading dashboard...</span>
                </div>
            </div>
        )
    }

    const cashBalance = funds?.cash_balance ?? 0
    const blockedFunds = funds?.blocked_funds ?? 0
    const availableBalance = cashBalance - blockedFunds
    const totalHoldings = holdings.length
    const recentOrders = history.slice(0, 5)

    return (
        <div className="page-container">
            <div className="page-header">
                <h1>Welcome back, {user?.name?.split('')[0] || 'Trader'} </h1>
                <p>Here's your portfolio overview and recent activity</p>
            </div>

            {/* Stats Grid */}
            <div className="grid-4" style={{ marginBottom: 'var(--space-xl)'}}>
                <StatCard
                    label="Available Balance"
                    value={fmt(availableBalance)}
                    sub="Ready to trade"
                    color="var(--color-buy)"
                />
                <StatCard
                    label="Total Balance"
                    value={fmt(cashBalance)}
                    sub="Including blocked"
                />
                <StatCard
                    label="Blocked Funds"
                    value={fmt(blockedFunds)}
                    sub="In open orders"
                    color="var(--color-warning)"
                />
                <StatCard
                    label="Holdings"
                    value={totalHoldings}
                    sub={`${totalHoldings} stock${totalHoldings !== 1 ? 's': ''} owned`}
                    color="var(--accent-primary)"
                />
            </div>

            {/* Two-column section */}
            <div className="grid-2">
                {/* Holdings Preview */}
                <div className="glass-card" style={{ padding: 'var(--space-lg)'}}>
                    <div className="section-header">
                        <h2>Holdings</h2>
                        <Link to="/portfolio" className="btn btn-ghost" style={{ padding: '6px 12px', fontSize: '0.75rem'}}>
                            View All →
                        </Link>
                    </div>
                    {holdings.length === 0 ? (
                        <div className="empty-state" style={{ padding: 'var(--space-xl)'}}>
                            <div className="empty-icon"></div>
                            <p>No holdings yet. Start trading!</p>
                        </div>
                    ) : (
                        <table className="data-table">
                            <thead>
                                <tr>
                                    <th>Symbol</th>
                                    <th>Quantity</th>
                                    <th>Blocked</th>
                                </tr>
                            </thead>
                            <tbody>
                                {holdings.slice(0, 5).map((h, i) =>(
                                    <tr key={i}>
                                        <td className="text-mono font-semibold text-accent">{h.symbol}</td>
                                        <td className="text-mono">{h.quantity?.toLocaleString()}</td>
                                        <td className="text-mono text-secondary">{h.blocked_qty?.toLocaleString()}</td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    )}
                </div>

                {/* Recent Orders */}
                <div className="glass-card" style={{ padding: 'var(--space-lg)'}}>
                    <div className="section-header">
                        <h2>Recent Orders</h2>
                        <Link to="/portfolio" className="btn btn-ghost" style={{ padding: '6px 12px', fontSize: '0.75rem'}}>
                            View All →
                        </Link>
                    </div>
                    {recentOrders.length === 0 ? (
                        <div className="empty-state" style={{ padding: 'var(--space-xl)'}}>
                            <div className="empty-icon"></div>
                            <p>No order history yet</p>
                        </div>
                    ) : (
                        <table className="data-table">
                            <thead>
                                <tr>
                                    <th>Symbol</th>
                                    <th>Side</th>
                                    <th>Status</th>
                                    <th>Qty</th>
                                </tr>
                            </thead>
                            <tbody>
                                {recentOrders.map((o, i) =>(
                                    <tr key={i}>
                                        <td className="text-mono font-semibold">{o.symbol}</td>
                                        <td>
                                            <span className={`badge ${o.side === 'BUY'? 'badge-success': 'badge-danger'}`}>
                                                {o.side}
                                            </span>
                                        </td>
                                        <td>
                                            <span className={`badge ${o.status === 'FILLED'? 'badge-success': o.status === 'REJECTED'? 'badge-danger': 'badge-warning'}`}>
                                                {o.status}
                                            </span>
                                        </td>
                                        <td className="text-mono">{o.quantity?.toLocaleString()}</td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    )}
                </div>
            </div>

            {/* Quick Actions */}
            <div style={{ marginTop: 'var(--space-xl)', display: 'flex', gap: 'var(--space-md)'}}>
                <Link to="/trade" className="btn btn-buy btn-lg">Place Order</Link>
                <Link to="/market" className="btn btn-ghost btn-lg">Browse Market</Link>
            </div>
        </div>
    )
}
