export default function StatCard({ label, value, subValue, color = 'default', mono = true }) {
  const colorMap = {
    default: 'text-text-primary',
    profit: 'text-profit',
    loss: 'text-loss',
    warn: 'text-warn',
    info: 'text-info',
  }
  const valueColor = colorMap[color] || colorMap.default

  return (
    <div className="surface rounded-sm p-5 flex flex-col gap-2">
      <p className="mono text-xs text-text-muted tracking-widest uppercase">{label}</p>
      <p className={`${mono ? 'mono' : 'font-sans'} text-xl font-medium ${valueColor}`}>
        {value}
      </p>
      {subValue && (
        <p className="mono text-xs text-text-muted">{subValue}</p>
      )}
    </div>
  )
}
