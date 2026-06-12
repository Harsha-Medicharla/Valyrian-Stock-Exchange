import { useState } from 'react'
import { AuthContext, getStoredAuth } from './auth-context'

export function AuthProvider({ children }) {
  const [authState, setAuthState] = useState(getStoredAuth)

  function login(newToken, newUser) {
    localStorage.setItem('nexus_token', newToken)
    localStorage.setItem('nexus_user', JSON.stringify(newUser))
    setAuthState({ token: newToken, user: newUser })
  }

  function logout() {
    localStorage.removeItem('nexus_token')
    localStorage.removeItem('nexus_user')
    setAuthState({ token: null, user: null })
  }

  return (
    <AuthContext.Provider
      value={{
        user: authState.user,
        token: authState.token,
        login,
        logout,
        isAuthenticated: !!authState.token,
      }}
    >
      {children}
    </AuthContext.Provider>
  )
}
