const API_BASE = '/api/v1';

function getToken() {
    return localStorage.getItem('val_token');
}

async function request(endpoint, options = {}) {
    const token = getToken();
    const headers = {
        'Content-Type': 'application/json',
        ...(token ? { Authorization: `Bearer ${token}` } : {}),
        ...options.headers,
    };

    const res = await fetch(`${API_BASE}${endpoint}`, {
        ...options,
        headers,
    });

    const data = await res.json();
    return { ok: res.ok, status: res.status, data };
}

// --- Auth ---
export async function apiLogin(email, password) {
    return request('/auth/login', {
        method: 'POST',
        body: JSON.stringify({ email, password }),
    });
}

export async function apiRegister(name, email, password) {
    return request('/auth/register', {
        method: 'POST',
        body: JSON.stringify({ name, email, password }),
    });
}

// --- Symbols ---
export async function getSymbols() {
    return request('/symbols');
}

// --- User Data (Protected) ---
export async function getUserFunds(userId) {
    return request(`/user/funds/${userId}`);
}

export async function getUserHoldings(userId) {
    return request(`/user/holdings/${userId}`);
}

export async function getUserInfo(userId) {
    return request(`/user/info/${userId}`);
}

export async function getUserHistory(userId) {
    return request(`/user/history/${userId}`);
}
