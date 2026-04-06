import { createContext, useContext, useState, useEffect } from 'react';
import { apiLogin, apiRegister, getUserInfo } from '../services/api';

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
    const [token, setToken] = useState(() =>localStorage.getItem('val_token'));
    const [userId, setUserId] = useState(() =>{
        const stored = localStorage.getItem('val_user_id');
        return stored ? parseInt(stored) : null;
    });
    const [user, setUser] = useState(null);
    const [loading, setLoading] = useState(true);

    // Load user info when we have a token and userId
    useEffect(() =>{
        if (token && userId) {
            getUserInfo(userId)
                .then(({ ok, data }) =>{
                    if (ok && data.status === 'success') {
                        setUser(data.data);
                    } else {
                        // Token invalid; logout
                        logout();
                    }
                })
                .catch(() =>logout())
                .finally(() =>setLoading(false));
        } else {
            setLoading(false);
        }
    }, [token, userId]);

    async function login(email, password) {
        const { ok, data } = await apiLogin(email, password);
        if (ok && data.status === 'success') {
            const t = data.data.token;
            const uid = data.data.user_id;
            localStorage.setItem('val_token', t);
            localStorage.setItem('val_user_id', uid);
            setToken(t);
            setUserId(uid);
            return { success: true };
        }
        return { success: false, message: data.message || 'Login failed'};
    }

    async function register(name, email, password) {
        const { ok, data } = await apiRegister(name, email, password);
        if (ok && data.status === 'success') {
            // Auto-login after registration
            return login(email, password);
        }
        return { success: false, message: data.message || 'Registration failed'};
    }

    function logout() {
        localStorage.removeItem('val_token');
        localStorage.removeItem('val_user_id');
        setToken(null);
        setUserId(null);
        setUser(null);
    }

    const isAuthenticated = !!token && !!userId;

    return (
        <AuthContext.Provider value={{ token, userId, user, isAuthenticated, loading, login, register, logout }}>
            {children}
        </AuthContext.Provider>
    );
}

export function useAuth() {
    const ctx = useContext(AuthContext);
    if (!ctx) throw new Error('useAuth must be used within an AuthProvider');
    return ctx;
}
