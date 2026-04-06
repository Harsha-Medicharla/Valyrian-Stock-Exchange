import { useState } from 'react'
import { useAuth } from '../context/AuthContext'

export default function LoginPage() {
    const { login, register } = useAuth()
    const [isLogin, setIsLogin] = useState(true)
    const [name, setName] = useState('')
    const [email, setEmail] = useState('')
    const [password, setPassword] = useState('')
    const [error, setError] = useState('')
    const [loading, setLoading] = useState(false)

    async function handleSubmit(e) {
        e.preventDefault()
        setError('')
        setLoading(true)

        try {
            const result = isLogin
                ? await login(email, password)
                : await register(name, email, password)

            if (!result.success) {
                setError(result.message)
            }
        } catch (err) {
            setError('Connection failed. Is the API server running?')
        } finally {
            setLoading(false)
        }
    }

    return (
        <div className="login-page">

            <div className="login-card glass-card">
                <div className="login-logo">
                    <div className="logo-mark">V</div>
                    <h1>Valyrian Exchange</h1>
                    <p>{isLogin ? 'Sign in to your trading account': 'Create your trading account'}</p>
                </div>

                {error && <div className="login-error">{error}</div>}

                <form className="login-form" onSubmit={handleSubmit}>
                    {!isLogin && (
                        <div className="form-group">
                            <label className="input-label" htmlFor="name">Full Name</label>
                            <input
                                id="name"
                                className="input-field"
                                type="text"
                                placeholder="John Doe"
                                value={name}
                                onChange={e =>setName(e.target.value)}
                                required={!isLogin}
                            />
                        </div>
                    )}

                    <div className="form-group">
                        <label className="input-label" htmlFor="email">Email Address</label>
                        <input
                            id="email"
                            className="input-field"
                            type="email"
                            placeholder="trader@example.com"
                            value={email}
                            onChange={e =>setEmail(e.target.value)}
                            required
                        />
                    </div>

                    <div className="form-group">
                        <label className="input-label" htmlFor="password">Password</label>
                        <input
                            id="password"
                            className="input-field"
                            type="password"
                            placeholder="••••••••"
                            value={password}
                            onChange={e =>setPassword(e.target.value)}
                            required
                        />
                    </div>

                    <button
                        className="btn btn-primary btn-lg"
                        type="submit"
                        disabled={loading}
                        style={{ width: '100%', marginTop: '8px'}}
                   >
                        {loading ? (
                            <><div className="spinner" style={{ width: 18, height: 18 }} />Processing...</>
                        ) : (
                            isLogin ? 'Sign In': 'Create Account'
                        )}
                    </button>
                </form>

                <div className="login-toggle">
                    {isLogin ? "Don't have an account?" : 'Already have an account?'}
                    <button onClick={() =>{ setIsLogin(!isLogin); setError('') }}>
                        {isLogin ? 'Register': 'Sign In'}
                    </button>
                </div>

                <div style={{
                    textAlign: 'center',
                    marginTop: '24px',
                    fontSize: '0.7rem',
                    color: 'var(--text-tertiary)'
                }}>
                    Valyrian Stock Exchange • Institutional-Grade Trading
                </div>
            </div>
        </div>
    )
}
