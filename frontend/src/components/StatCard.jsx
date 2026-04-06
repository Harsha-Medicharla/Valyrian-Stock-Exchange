export default function StatCard({ label, value, sub, color }) {
    return (
        <div className="glass-card stat-card">
            <div className="stat-label">{label}</div>
            <div className="stat-value" style={color ? { color } : {}}>
                {value}
            </div>
            {sub && <div className="stat-sub">{sub}</div>}
        </div>
    )
}
