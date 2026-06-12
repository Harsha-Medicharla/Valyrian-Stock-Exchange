import { useAuth } from '../context/useAuth'
import { useNavigate } from 'react-router-dom'

export default function TopBar() {
  const { user, logout } = useAuth()
  const navigate = useNavigate()

  function handleLogout() {
    logout()
    navigate('/login')
  }

  return (
    <header className="fixed top-0 left-0 right-0 z-50 flex items-center justify-between px-6 h-14 bg-bg border-b border-border">
      {/* Left: Logo */}
      <div className="flex items-center gap-3">
        <span className="mono text-base font-medium tracking-widest text-text-primary">NEXUS</span>
        <span className="mono text-xs text-text-muted tracking-[0.25em] hidden sm:block">EXCHANGE ENGINE</span>
      </div>

      {/* Right: User + role badge + Logout */}
      <div className="flex items-center gap-3">
        {user && (
          <>
            <span className="mono text-xs text-text-muted hidden sm:block">
              {user.name.toUpperCase()}
            </span>
            {user.role === 'ADMIN' && (
              <span className="badge-warn hidden sm:inline">ADMIN</span>
            )}
          </>
        )}
        <button
          onClick={handleLogout}
          className="mono text-xs text-text-muted hover:text-loss transition-colors tracking-wider border border-border px-3 py-1.5 hover:border-loss"
        >
          [LOGOUT]
        </button>
      </div>
    </header>
  )
}
