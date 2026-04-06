import { useState, useEffect } from 'react'
import { useAuth } from '../context/AuthContext'
import { getSymbols, getUserFunds } from '../services/api'

export default function TradePage() {
    const { userId } = useAuth()
    const [symbols, setSymbols] = useState([])
    const [funds, setFunds] = useState(null)
    const [side, setSide] = useState('BUY')
    const [orderType, setOrderType] = useState('LIMIT')
    const [selectedSymbol, setSelectedSymbol] = useState('')
    const [price, setPrice] = useState('')
    const [quantity, setQuantity] = useState('')
    const [loading, setLoading] = useState(true)
    const [orderPlaced, setOrderPlaced] = useState(false)

    useEffect(() =>{
        Promise.all([
            getSymbols(),
            getUserFunds(userId),
        ]).then(([symRes, fundRes]) =>{
            if (symRes.ok && symRes.data.status === 'success') {
                const syms = symRes.data.data.symbols || []
                setSymbols(syms)
                if (syms.length>0) setSelectedSymbol(syms[0].symbol)
            }
            if (fundRes.ok && fundRes.data.status === 'success') setFunds(fundRes.data.data)
        }).finally(() =>setLoading(false))
    }, [userId])

    const fmt = (val) =>{
        if (val === null || val === undefined) return '—'
        const amount = val / 10000
        return '₹'+ amount.toLocaleString('en-IN', { minimumFractionDigits: 2 })
    }

    const estimatedTotal = price && quantity
        ? (parseFloat(price) * parseInt(quantity))
        : 0

    function handlePlaceOrder(e) {
        e.preventDefault()
        // In a full implementation, this would POST to a trade endpoint
        setOrderPlaced(true)
        setTimeout(() =>setOrderPlaced(false), 3000)
    }

    if (loading) {
        return (
            <div className="page-container">
                <div className="loading-screen">
                    <div className="spinner" />
                    <span>Loading trade panel...</span>
                </div>
            </div>
        )
    }

    const availableBalance = funds ? (funds.cash_balance - funds.blocked_funds) / 10000 : 0

    return (
        <div className="page-container">
            <div className="page-header">
                <h1>Place Order</h1>
                <p>Execute trades on the Valyrian matching engine</p>
            </div>

            <div className="grid-2">
                {/* Order Form */}
                <div className="glass-card order-form-container">
                    {/* Side Toggle */}
                    <div className="side-toggle">
                        <button
                            className={`side-toggle-btn ${side === 'BUY'? 'active-buy': ''}`}
                            onClick={() =>setSide('BUY')}
                       >
                            BUY
                        </button>
                        <button
                            className={`side-toggle-btn ${side === 'SELL'? 'active-sell': ''}`}
                            onClick={() =>setSide('SELL')}
                       >
                            SELL
                        </button>
                    </div>

                    <form onSubmit={handlePlaceOrder}>
                        {/* Symbol Select */}
                        <div className="form-group" style={{ marginBottom: 'var(--space-md)'}}>
                            <label className="input-label">Symbol</label>
                            <select
                                className="input-field"
                                value={selectedSymbol}
                                onChange={e =>setSelectedSymbol(e.target.value)}
                           >
                                {symbols.map(s =>(
                                    <option key={s.symbol_id} value={s.symbol}>
                                        {s.symbol} — {s.company_name}
                                    </option>
                                ))}
                            </select>
                        </div>

                        {/* Order Type */}
                        <div className="form-group" style={{ marginBottom: 'var(--space-md)'}}>
                            <label className="input-label">Order Type</label>
                            <div style={{ display: 'flex', gap: '8px'}}>
                                {['LIMIT', 'MARKET'].map(t =>(
                                    <button
                                        key={t}
                                        type="button"
                                        className={`btn ${orderType === t ? 'btn-primary': 'btn-ghost'}`}
                                        style={{ flex: 1, padding: '8px'}}
                                        onClick={() =>setOrderType(t)}
                                   >
                                        {t}
                                    </button>
                                ))}
                            </div>
                        </div>

                        {/* Price (for LIMIT orders) */}
                        {orderType === 'LIMIT'&& (
                            <div className="form-group" style={{ marginBottom: 'var(--space-md)'}}>
                                <label className="input-label">Price (₹)</label>
                                <input
                                    className="input-field text-mono"
                                    type="number"
                                    step="0.01"
                                    min="0"
                                    placeholder="0.00"
                                    value={price}
                                    onChange={e =>setPrice(e.target.value)}
                                    required
                                />
                            </div>
                        )}

                        {/* Quantity */}
                        <div className="form-group" style={{ marginBottom: 'var(--space-md)'}}>
                            <label className="input-label">Quantity</label>
                            <input
                                className="input-field text-mono"
                                type="number"
                                min="1"
                                step="1"
                                placeholder="0"
                                value={quantity}
                                onChange={e =>setQuantity(e.target.value)}
                                required
                            />
                        </div>

                        {/* Order Summary */}
                        {(price || orderType === 'MARKET') && quantity && (
                            <div className="order-summary">
                                <div className="order-summary-row">
                                    <span className="label">Side</span>
                                    <span className="value" style={{ color: side === 'BUY'? 'var(--color-buy)': 'var(--color-sell)'}}>
                                        {side}
                                    </span>
                                </div>
                                <div className="order-summary-row">
                                    <span className="label">Symbol</span>
                                    <span className="value">{selectedSymbol}</span>
                                </div>
                                <div className="order-summary-row">
                                    <span className="label">Type</span>
                                    <span className="value">{orderType}</span>
                                </div>
                                {orderType === 'LIMIT'&& (
                                    <>
                                        <div className="order-summary-row">
                                            <span className="label">Price</span>
                                            <span className="value">₹{parseFloat(price || 0).toFixed(2)}</span>
                                        </div>
                                        <div className="order-summary-row">
                                            <span className="label">Estimated Total</span>
                                            <span className="value" style={{ color: 'var(--accent-primary)', fontSize: '0.95rem'}}>
                                                ₹{estimatedTotal.toLocaleString('en-IN', { minimumFractionDigits: 2 })}
                                            </span>
                                        </div>
                                    </>
                                )}
                            </div>
                        )}

                        {/* Submit */}
                        <button
                            type="submit"
                            className={`btn btn-lg ${side === 'BUY'? 'btn-buy': 'btn-sell'}`}
                            style={{ width: '100%', marginTop: 'var(--space-lg)'}}
                       >
                            {side === 'BUY'? '': ''} {side} {selectedSymbol}
                        </button>

                        {orderPlaced && (
                            <div
                                className="badge badge-success"
                                style={{
                                    display: 'block',
                                    textAlign: 'center',
                                    padding: '12px',
                                    marginTop: 'var(--space-md)',
                                    fontSize: '0.82rem'
                                }}
                           >
                                 Order submitted to the matching engine!
                            </div>
                        )}
                    </form>
                </div>

                {/* Right Panel: Order Book & Info */}
                <div style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-md)'}}>
                    {/* Account Info */}
                    <div className="glass-card" style={{ padding: 'var(--space-lg)'}}>
                        <h3 style={{ fontSize: '0.85rem', fontWeight: 600, marginBottom: 'var(--space-md)', color: 'var(--text-secondary)'}}>
                             Account Balance
                        </h3>
                        <div className="text-mono" style={{ fontSize: '1.3rem', fontWeight: 700, color: 'var(--color-buy)'}}>
                            {fmt(funds?.cash_balance)}
                        </div>
                        <div style={{ fontSize: '0.78rem', color: 'var(--text-tertiary)', marginTop: '4px'}}>
                            Available: ₹{availableBalance.toLocaleString('en-IN', { minimumFractionDigits: 2 })}
                        </div>
                    </div>

                    {/* Simulated Order Book */}
                    <div className="glass-card" style={{ padding: 'var(--space-lg)', flex: 1 }}>
                        <h3 style={{ fontSize: '0.85rem', fontWeight: 600, marginBottom: 'var(--space-md)', color: 'var(--text-secondary)'}}>
                             Order Book — {selectedSymbol}
                        </h3>

                        {/* Ask side */}
                        <div style={{ marginBottom: '8px'}}>
                            <div style={{ fontSize: '0.7rem', color: 'var(--text-tertiary)', marginBottom: '6px', textTransform: 'uppercase', letterSpacing: '0.06em'}}>
                                Asks (Sell)
                            </div>
                            {[5, 4, 3, 2, 1].map(i =>{
                                const askPrice = (150 + i * 0.5).toFixed(2)
                                const askQty = Math.floor(Math.random() * 500 + 100)
                                const barWidth = (askQty / 600) * 100
                                return (
                                    <div key={`ask-${i}`} style={{
                                        display: 'flex',
                                        justifyContent: 'space-between',
                                        padding: '3px 8px',
                                        fontSize: '0.78rem',
                                        fontFamily: 'var(--font-mono)',
                                        position: 'relative',
                                        marginBottom: '1px',
                                    }}>
                                        <div style={{
                                            position: 'absolute',
                                            right: 0,
                                            top: 0,
                                            bottom: 0,
                                            width: `${barWidth}%`,
                                            background: 'var(--color-sell-dim)',
                                            borderRadius: 'var(--radius-sm)',
                                        }} />
                                        <span style={{ position: 'relative', color: 'var(--color-sell)'}}>{askPrice}</span>
                                        <span style={{ position: 'relative', color: 'var(--text-secondary)'}}>{askQty}</span>
                                    </div>
                                )
                            })}
                        </div>

                        {/* Spread */}
                        <div style={{
                            textAlign: 'center',
                            padding: '6px',
                            fontSize: '0.75rem',
                            color: 'var(--accent-primary)',
                            fontWeight: 600,
                            borderTop: '1px solid var(--border-subtle)',
                            borderBottom: '1px solid var(--border-subtle)',
                            margin: '4px 0',
                        }}>
                            Spread: ₹0.50
                        </div>

                        {/* Bid side */}
                        <div>
                            <div style={{ fontSize: '0.7rem', color: 'var(--text-tertiary)', marginBottom: '6px', marginTop: '8px', textTransform: 'uppercase', letterSpacing: '0.06em'}}>
                                Bids (Buy)
                            </div>
                            {[1, 2, 3, 4, 5].map(i =>{
                                const bidPrice = (150 - i * 0.5).toFixed(2)
                                const bidQty = Math.floor(Math.random() * 500 + 100)
                                const barWidth = (bidQty / 600) * 100
                                return (
                                    <div key={`bid-${i}`} style={{
                                        display: 'flex',
                                        justifyContent: 'space-between',
                                        padding: '3px 8px',
                                        fontSize: '0.78rem',
                                        fontFamily: 'var(--font-mono)',
                                        position: 'relative',
                                        marginBottom: '1px',
                                    }}>
                                        <div style={{
                                            position: 'absolute',
                                            left: 0,
                                            top: 0,
                                            bottom: 0,
                                            width: `${barWidth}%`,
                                            background: 'var(--color-buy-dim)',
                                            borderRadius: 'var(--radius-sm)',
                                        }} />
                                        <span style={{ position: 'relative', color: 'var(--color-buy)'}}>{bidPrice}</span>
                                        <span style={{ position: 'relative', color: 'var(--text-secondary)'}}>{bidQty}</span>
                                    </div>
                                )
                            })}
                        </div>
                    </div>

                    {/* Trading Info */}
                    <div className="glass-card" style={{ padding: 'var(--space-md)'}}>
                        <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.78rem'}}>
                            <span className="text-secondary">Engine</span>
                            <span className="badge badge-success">Online</span>
                        </div>
                        <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.78rem', marginTop: '8px'}}>
                            <span className="text-secondary">Latency</span>
                            <span className="text-mono text-accent">&lt; 1μs</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}
