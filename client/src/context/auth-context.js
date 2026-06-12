import { createContext } from 'react'

export const AuthContext = createContext(null)

export function getStoredAuth() {
  const storedToken = localStorage.getItem('nexus_token')
  const storedUser = localStorage.getItem('nexus_user')

  if (!storedToken || !storedUser) {
    return { token: null, user: null }
  }

  try {
    return {
      token: storedToken,
      user: JSON.parse(storedUser),
    }
  } catch {
    localStorage.removeItem('nexus_token')
    localStorage.removeItem('nexus_user')
    return { token: null, user: null }
  }
}
