import { useState } from 'react'
import { Link, useNavigate } from 'react-router-dom'
import api from '../api/axios'
import { useAuth } from '../context/useAuth'

export default function RegisterPage() {
  const { login } = useAuth()
  const navigate = useNavigate()
  const [form, setForm] = useState({ name: '', email: '', password: '' })
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(false)

  function updateField(key, value) {
    setForm((current) => ({ ...current, [key]: value }))
  }

  async function handleSubmit(e) {
    e.preventDefault()
    setError('')

    if (!form.name || !form.email || !form.password) {
      setError('Name, email, and password are required.')
      return
    }

    setLoading(true)

    try {
      const res = await api.post('/api/auth/register', form)
      login(res.data.token, res.data.user)
      navigate('/dashboard')
    } catch (err) {
      setError(err.response?.data?.error || 'Registration failed.')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="min-h-screen bg-bg flex items-center justify-center px-4">
      <div className="w-full max-w-sm">
        <div className="mb-10 text-center">
          <p className="mono text-2xl font-medium tracking-widest text-text-primary">NEXUS</p>
          <p className="mono text-xs text-text-muted tracking-[0.3em] mt-1">EXCHANGE ENGINE</p>
        </div>

        <div className="surface rounded-sm p-8">
          <h1 className="text-sm font-medium text-text-muted mb-6 tracking-widest uppercase">Create Account</h1>

          <form onSubmit={handleSubmit} className="space-y-4">
            <div>
              <label className="block text-xs text-text-muted mono mb-1.5 tracking-wider">NAME</label>
              <input
                type="text"
                value={form.name}
                onChange={(e) => updateField('name', e.target.value)}
                placeholder="Arjun Mehta"
                className="w-full px-3 py-2.5 text-sm rounded-sm mono"
                autoComplete="name"
              />
            </div>

            <div>
              <label className="block text-xs text-text-muted mono mb-1.5 tracking-wider">EMAIL</label>
              <input
                type="email"
                value={form.email}
                onChange={(e) => updateField('email', e.target.value)}
                placeholder="trader@nexus.io"
                className="w-full px-3 py-2.5 text-sm rounded-sm mono"
                autoComplete="email"
              />
            </div>

            <div>
              <label className="block text-xs text-text-muted mono mb-1.5 tracking-wider">PASSWORD</label>
              <input
                type="password"
                value={form.password}
                onChange={(e) => updateField('password', e.target.value)}
                placeholder="Minimum 6 characters"
                className="w-full px-3 py-2.5 text-sm rounded-sm mono"
                autoComplete="new-password"
              />
            </div>

            {error && <p className="text-loss text-xs mono">{error}</p>}

            <button
              type="submit"
              disabled={loading}
              className="w-full mt-2 bg-info text-white mono text-sm font-medium py-2.5 rounded-sm hover:bg-blue-500 transition-colors disabled:opacity-50 disabled:cursor-not-allowed tracking-wider"
            >
              {loading ? 'CREATING ACCOUNT...' : 'REGISTER →'}
            </button>
          </form>
        </div>

        <p className="text-center text-xs text-text-muted mono mt-6">
          HAVE AN ACCOUNT? <Link to="/login" className="text-info">SIGN IN</Link>
        </p>
      </div>
    </div>
  )
}
