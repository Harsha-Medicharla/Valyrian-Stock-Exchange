import { NavLink, useLocation } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'

const navItems = [
    { path: '/', label: 'Dashboard', icon: ''},
    { path: '/market', label: 'Market', icon: ''},
    { path: '/trade', label: 'Trade', icon: ''},
    { path: '/portfolio', label: 'Portfolio', icon: ''},
]

export default function Sidebar() {
    const { user, logout } = useAuth()
    const location = useLocation()

    return (
        <aside className="sidebar">
            <div className="sidebar-logo">
                <div className="logo-icon">V</div>
                <div>
                    <div className="logo-text">Valyrian</div>
                    <div className="logo-sub">Stock Exchange</div>
                </div>
            </div>

            <nav className="sidebar-nav">
                <div className="sidebar-section-label">Navigation</div>
                {navItems.map(item =>(
                    <NavLink
                        key={item.path}
                        to={item.path}
                        className={({ isActive }) =>
                            `sidebar-link ${isActive ? 'active': ''}`
                        }
                        end={item.path === '/'}
                   >
                        <span className="link-icon">{item.icon}</span>
                        <span>{item.label}</span>
                    </NavLink>
                ))}

                <div className="sidebar-section-label" style={{ marginTop: '24px'}}>Account</div>
                <div className="sidebar-link" style={{ cursor: 'default'}}>
                    <span className="link-icon"></span>
                    <span>{user?.name || 'User'}</span>
                </div>
            </nav>

            <div className="sidebar-footer">
                <button
                    className="btn btn-ghost"
                    style={{ width: '100%'}}
                    onClick={logout}
               >
                     Sign Out
                </button>
            </div>
        </aside>
    )
}
