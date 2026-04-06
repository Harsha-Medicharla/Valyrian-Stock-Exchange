import { useAuth } from '../context/AuthContext'
import { useLocation } from 'react-router-dom'
import { useState, useEffect } from 'react'
import { getUserFunds } from '../services/api'

const pageTitles = {
    '/': 'Dashboard',
    '/market': 'Market',
    '/trade': 'Trade',
    '/portfolio': 'Portfolio',
}

export default function Navbar() {
    const { user, userId, logout } = useAuth()
    const location = useLocation()
    const [balance, setBalance] = useState(null)

    useEffect(() =>{
        if (userId) {
            getUserFunds(userId).then(({ ok, data }) =>{
                if (ok && data.status === 'success') {
                    setBalance(data.data.cash_balance)
                }
            })
        }
    }, [userId, location.pathname])

    const formatCurrency = (val) =>{
        if (val === null || val === undefined) return '...'
        // Values stored as integer cents (paise) — divide by 10000 for display
        const amount = val / 10000
        return '₹'+ amount.toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
    }

    const title = pageTitles[location.pathname] || 'Valyrian Exchange'
    const initials = user?.name ? user.name.split('').map(n =>n[0]).join('').toUpperCase().slice(0, 2) : '?'

    return (
        <nav className="navbar">
            <div className="navbar-left">
                <h2 style={{ fontSize: '1.1rem', fontWeight: 600 }}>{title}</h2>
            </div>
            <div className="navbar-right">
                <div className="balance-pill">
                    <span></span>
                    <span>{formatCurrency(balance)}</span>
                </div>
                <div className="user-avatar" title={user?.name}>{initials}</div>
            </div>
        </nav>
    )
}
