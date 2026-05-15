import { useCallback, useEffect, useRef, useState } from 'react'

type MetricsPanelProps = {
  serverConnected: boolean
}

type Metrics = {
  cpuPercent: number
  memPercent: number
  memUsageMB: number
  memLimitMB: number
  connections: number
  threads: number
  requestsPerSec: number
}

type HistoryPoint = {
  time: string
  value: number
}

type MetricCardProps = {
  label: string
  value: string
  subValue?: string
  percent: number
  color: string
  history: HistoryPoint[]
}

const maxHistory = 30

export default function MetricsPanel({ serverConnected }: MetricsPanelProps) {
  const [metrics, setMetrics] = useState<Metrics | null>(null)
  const [cpuHistory, setCpuHistory] = useState<HistoryPoint[]>([])
  const [memHistory, setMemHistory] = useState<HistoryPoint[]>([])
  const [error, setError] = useState<string | null>(null)
  const intervalRef = useRef<number | null>(null)

  const fetchMetrics = useCallback(async () => {
    if (!serverConnected) return

    try {
      const response = await fetch('/api/server/metrics')

      if (!response.ok) {
        throw new Error('Metrics API is not available')
      }

      const data = (await response.json()) as Partial<Metrics>
      const nextMetrics: Metrics = {
        cpuPercent: data.cpuPercent ?? 0,
        memPercent: data.memPercent ?? 0,
        memUsageMB: data.memUsageMB ?? 0,
        memLimitMB: data.memLimitMB ?? 0,
        connections: data.connections ?? 0,
        threads: data.threads ?? 0,
        requestsPerSec: data.requestsPerSec ?? 0,
      }
      const now = new Date().toLocaleTimeString('ko-KR', { hour12: false })

      setMetrics(nextMetrics)
      setError(null)
      setCpuHistory((items) => trimHistory([...items, { time: now, value: nextMetrics.cpuPercent }]))
      setMemHistory((items) => trimHistory([...items, { time: now, value: nextMetrics.memPercent }]))
    } catch (caught) {
      setError(caught instanceof Error ? caught.message : 'Metrics request failed')
    }
  }, [serverConnected])

  useEffect(() => {
    if (!serverConnected) {
      window.queueMicrotask(() => {
        setMetrics(null)
        setCpuHistory([])
        setMemHistory([])
        setError(null)
      })

      return
    }

    window.setTimeout(() => {
      void fetchMetrics()
    }, 0)

    intervalRef.current = window.setInterval(() => {
      void fetchMetrics()
    }, 3000)

    return () => {
      if (intervalRef.current !== null) {
        window.clearInterval(intervalRef.current)
      }
    }
  }, [fetchMetrics, serverConnected])

  if (!serverConnected) {
    return (
      <div className="glass-card" style={{ padding: '22px 24px' }}>
        <div className="card-title">Metrics</div>
        <div className="terminal-empty">서버에 연결되면 메트릭이 표시됩니다.</div>
      </div>
    )
  }

  return (
    <div className="glass-card" style={{ padding: '22px 24px' }}>
      <div className="card-header" style={{ marginBottom: 16 }}>
        <div className="card-title">Metrics</div>
        {error && <span className="message-text error">{error}</span>}
      </div>

      <div className="metrics-grid">
        <MetricCard
          label="CPU"
          value={`${(metrics?.cpuPercent ?? 0).toFixed(1)}%`}
          percent={metrics?.cpuPercent ?? 0}
          color="#38bdf8"
          history={cpuHistory}
        />
        <MetricCard
          label="Memory"
          value={`${metrics?.memUsageMB ?? 0} MB`}
          subValue={metrics?.memLimitMB ? `/ ${metrics.memLimitMB} MB` : undefined}
          percent={metrics?.memPercent ?? 0}
          color="#22c55e"
          history={memHistory}
        />
      </div>

      <div className="mini-stat-grid">
        <MiniStat label="Connections" value={(metrics?.connections ?? 0).toString()} />
        <MiniStat label="Threads" value={(metrics?.threads ?? 0).toString()} />
        <MiniStat label="Req/s" value={(metrics?.requestsPerSec ?? 0).toString()} />
      </div>
    </div>
  )
}

function MetricCard({ label, value, subValue, percent, color, history }: MetricCardProps) {
  const barColor = percent > 80 ? 'var(--color-stopped)' : percent > 60 ? 'var(--color-warning)' : color

  return (
    <div className="metric-card">
      <div className="metric-card-top">
        <span className="metric-label">{label}</span>
        <span className="metric-value" style={{ color }}>
          {value}
        </span>
      </div>
      {subValue && <div className="metric-subvalue">{subValue}</div>}
      <div className="progress-bar" style={{ marginBottom: 10 }}>
        <div
          className="progress-fill"
          style={{ width: `${Math.min(percent, 100)}%`, background: barColor, boxShadow: `0 0 8px ${color}` }}
        />
      </div>
      <div style={{ height: 44 }}>
        <Sparkline color={color} points={history} />
      </div>
    </div>
  )
}

function MiniStat({ label, value }: { label: string; value: string }) {
  return (
    <div className="mini-stat">
      <span>{label}</span>
      <strong>{value}</strong>
    </div>
  )
}

function trimHistory(items: HistoryPoint[]) {
  return items.length > maxHistory ? items.slice(-maxHistory) : items
}

function Sparkline({ color, points }: { color: string; points: HistoryPoint[] }) {
  const width = 240
  const height = 44
  const values = points.length > 1 ? points : [{ time: '', value: 0 }, { time: '', value: 0 }]
  const step = width / Math.max(values.length - 1, 1)
  const path = values
    .map((point, index) => {
      const x = index * step
      const y = height - (Math.max(0, Math.min(point.value, 100)) / 100) * height

      return `${x.toFixed(1)},${y.toFixed(1)}`
    })
    .join(' ')

  return (
    <svg aria-hidden="true" height="100%" preserveAspectRatio="none" viewBox={`0 0 ${width} ${height}`} width="100%">
      <polyline fill="none" points={path} stroke={color} strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" />
    </svg>
  )
}
