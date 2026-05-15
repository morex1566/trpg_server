type StatusBadgeProps = {
  state: 'connected' | 'disconnected' | 'connecting' | 'running' | 'stopped' | 'paused'
  size?: 'sm' | 'md'
}

export default function StatusBadge({ state, size = 'md' }: StatusBadgeProps) {
  const normalized = state.toLowerCase()
  const running = normalized === 'connected' || normalized === 'running'
  const warning = normalized === 'connecting' || normalized === 'paused'
  const stopped = normalized === 'disconnected' || normalized === 'stopped'
  const className = `badge ${running ? 'badge-running' : warning ? 'badge-warning' : stopped ? 'badge-stopped' : 'badge-default'}`
  const label = running ? 'Online' : warning ? normalized : 'Offline'

  return (
    <span className={className} style={{ fontSize: size === 'sm' ? 10 : undefined }}>
      <span className={`pulse-dot ${running || warning ? 'active' : ''}`} />
      {label}
    </span>
  )
}
