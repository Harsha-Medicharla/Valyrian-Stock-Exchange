import { useState } from 'react'
import { Link, useNavigate } from 'react-router-dom'
import { useAuth } from '../context/useAuth'
import api from '../api/axios'

export default function LoginPage() {
  const { login } = useAuth()
  const navigate = useNavigate()
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(false)

  async function handleSubmit(e) {
    e.preventDefault()
    setError('')
    if (!email || !password) {
      setError('Email and password are required.')
      return
    }
    setLoading(true)
    try {
      const res = await api.post('/api/auth/login', { email, password })
      login(res.data.token, res.data.user)
      navigate('/dashboard')
    } catch (err) {
      setError(err.response?.data?.error || 'Authentication failed.')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="min-h-screen bg-bg flex items-center justify-center px-4">
      <div className="w-full max-w-sm">
        {/* Logo */}
        <div className="mb-10 text-center">
          <p className="mono text-2xl font-medium tracking-widest text-text-primary">NEXUS</p>
          <p className="mono text-xs text-text-muted tracking-[0.3em] mt-1">EXCHANGE ENGINE</p>
        </div>

        {/* Card */}
        <div className="surface rounded-sm p-8">
          <h1 className="text-sm font-medium text-text-muted mb-6 tracking-widest uppercase">Sign In</h1>

          <form onSubmit={handleSubmit} className="space-y-4">
            <div>
              <label className="block text-xs text-text-muted mono mb-1.5 tracking-wider">EMAIL</label>
              <input
                type="email"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                placeholder="trader@nexus.io"
                className="w-full px-3 py-2.5 text-sm rounded-sm mono"
                autoComplete="email"
              />
            </div>

            <div>
              <label className="block text-xs text-text-muted mono mb-1.5 tracking-wider">PASSWORD</label>
              <input
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="••••••••"
                className="w-full px-3 py-2.5 text-sm rounded-sm mono"
                autoComplete="current-password"
              />
            </div>

            {error && (
              <p className="text-loss text-xs mono">{error}</p>
            )}

            <button
              type="submit"
              disabled={loading}
              className="w-full mt-2 bg-info text-white mono text-sm font-medium py-2.5 rounded-sm hover:bg-blue-500 transition-colors disabled:opacity-50 disabled:cursor-not-allowed tracking-wider"
            >
              {loading ? 'AUTHENTICATING...' : 'SIGN IN →'}
            </button>
          </form>
        </div>

        <p className="text-center text-xs text-text-muted mono mt-6">
          ACID-GUARANTEED TRADE EXECUTION · <Link to="/register" className="text-info">CREATE ACCOUNT</Link>
        </p>
      </div>
    </div>
  )
}
