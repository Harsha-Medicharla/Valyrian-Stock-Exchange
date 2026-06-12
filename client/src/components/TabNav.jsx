import { useAuth } from '../context/useAuth'

export default function TabNav({ activeTab, onTabChange }) {
  const { user } = useAuth()

  const tabs = [
    { id: 'trader',    label: 'TRADER VIEW',     visible: true },
    { id: 'analytics', label: 'DB ANALYTICS',    visible: true },
    { id: 'admin',     label: 'ADMIN / MONITOR', visible: user?.role === 'ADMIN' },
  ].filter(t => t.visible)

  return (
    <nav className="flex border-b border-border bg-bg">
      {tabs.map((tab) => (
        <button
          key={tab.id}
          onClick={() => onTabChange(tab.id)}
          className={`mono text-xs tracking-widest px-6 py-3.5 transition-colors border-b-2 ${
            activeTab === tab.id
              ? 'border-info text-info'
              : 'border-transparent text-text-muted hover:text-text-primary'
          }`}
        >
          {tab.label}
        </button>
      ))}
    </nav>
  )
}
