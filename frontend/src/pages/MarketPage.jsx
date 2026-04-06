import { useState, useEffect } from 'react'
import { getSymbols } from '../services/api'
import { Link } from 'react-router-dom'

export default function MarketPage() {
    const [symbols, setSymbols] = useState([])
    const [search, setSearch] = useState('')
    const [loading, setLoading] = useState(true)

    useEffect(() =>{
        getSymbols().then(({ ok, data }) =>{
            if (ok && data.status === 'success') {
                setSymbols(data.data.symbols || [])
            }
        }).finally(() =>setLoading(false))
    }, [])

    const filtered = symbols.filter(s =>
        s.symbol.toLowerCase().includes(search.toLowerCase()) ||
        s.company_name.toLowerCase().includes(search.toLowerCase())
    )

    if (loading) {
        return (
            <div className="page-container">
                <div className="loading-screen">
                    <div className="spinner" />
                    <span>Loading market data...</span>
                </div>
            </div>
        )
    }

    return (
        <div className="page-container">
            <div className="page-header">
                <h1>Market Overview</h1>
                <p>Browse all listed instruments on the Valyrian Stock Exchange</p>
            </div>

            {/* Search */}
            <div className="search-bar" style={{ maxWidth: '400px', marginBottom: 'var(--space-xl)'}}>
                <span className="search-icon"></span>
                <input
                    type="text"
                    placeholder="Search symbols or companies..."
                    value={search}
                    onChange={e =>setSearch(e.target.value)}
                />
            </div>

            {/* Stats Bar */}
            <div style={{
                display: 'flex',
                gap: 'var(--space-lg)',
                marginBottom: 'var(--space-xl)',
                fontSize: '0.82rem',
                color: 'var(--text-secondary)'
            }}>
                <span><strong style={{ color: 'var(--accent-primary)'}}>{symbols.length}</strong>Total Instruments</span>
                <span><strong style={{ color: 'var(--text-primary)'}}>{filtered.length}</strong>Showing</span>
            </div>

            {/* Symbol Grid */}
            {filtered.length === 0 ? (
                <div className="empty-state">
                    <div className="empty-icon"></div>
                    <p>No symbols found matching "{search}"</p>
                </div>
            ) : (
                <div className="grid-4">
                    {filtered.map(s =>(
                        <Link to="/trade" key={s.symbol_id} style={{ textDecoration: 'none'}}>
                            <div className="glass-card symbol-card">
                                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start'}}>
                                    <div>
                                        <div className="symbol-ticker">{s.symbol}</div>
                                        <div className="symbol-name">{s.company_name}</div>
                                    </div>
                                    <div className="symbol-exchange">{s.exchange}</div>
                                </div>
                                <div style={{
                                    marginTop: 'var(--space-md)',
                                    height: '32px',
                                    background: 'linear-gradient(90deg, var(--accent-primary-dim), transparent)',
                                    borderRadius: 'var(--radius-sm)',
                                    display: 'flex',
                                    alignItems: 'center',
                                    justifyContent: 'center',
                                    fontSize: '0.7rem',
                                    color: 'var(--text-tertiary)'
                                }}>
                                    Click to trade →
                                </div>
                            </div>
                        </Link>
                    ))}
                </div>
            )}
        </div>
    )
}
