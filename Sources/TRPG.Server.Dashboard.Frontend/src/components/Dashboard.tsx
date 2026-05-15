import { BarChart3, Database, Network, RefreshCw, Server, Terminal, Zap } from 'lucide-react'
import type { ReactNode } from 'react'
import type { ConnectionInfo } from '../api/serverStatus'
import ConnectionSettings from './ConnectionSettings'
import LogViewer from './LogViewer'
import MetricsPanel from './MetricsPanel'
import ServerStatus from './ServerStatus'

type DashboardProps = {
  connected: boolean
  connectionInfo: ConnectionInfo | null
  loading: boolean
  lastUpdated: Date | null
  onRefresh: () => void | Promise<void>
  onConnect: () => void | Promise<void>
  onDisconnect: () => void | Promise<void>
}

type StatCardProps = {
  icon: ReactNode
  label: string
  value: string
  color?: string
  glow?: boolean
}

type SectionTitleProps = {
  icon: ReactNode
  title: string
  subtitle: string
}

export default function Dashboard({
  connected,
  connectionInfo,
  loading,
  lastUpdated,
  onRefresh,
  onConnect,
  onDisconnect,
}: DashboardProps) {
  return (
    <div className="dashboard-page">
      <header className="dashboard-topbar">
        <div className="topbar-inner">
          <div className="flex items-center gap-3">
            <div className="brand-mark">
              <Zap size={18} strokeWidth={2.4} />
            </div>
            <div>
              <div className="brand-title">TRPG.Server Dashboard</div>
              <div className="brand-subtitle">HTTP Control Surface</div>
            </div>
          </div>

          <div className="topbar-meta">
            {connected && connectionInfo && (
              <div className="connection-hint">
                <strong>TCP Connected</strong>
                <span className="monospace-value">
                  {connectionInfo.host}:{connectionInfo.port}
                </span>
              </div>
            )}
            {lastUpdated && (
              <span className="last-updated">
                {lastUpdated.toLocaleTimeString('ko-KR', { hour12: false })}
              </span>
            )}
            <button className="btn btn-ghost btn-sm" type="button" onClick={() => void onRefresh()}>
              {loading ? <span className="spinner" /> : <RefreshCw size={14} />}
              Refresh
            </button>
          </div>
        </div>
      </header>

      <main className="dashboard-main">
        {!loading && (
          <section className="stats-bar" aria-label="Server summary">
            <StatCard
              icon={<Server size={15} />}
              label="TCP"
              value={connected ? 'Connected' : 'Disconnected'}
              color={connected ? 'var(--color-running)' : 'var(--color-stopped)'}
              glow={connected}
            />
            <StatCard
              icon={<Network size={15} />}
              label="Endpoint"
              value={connectionInfo?.label ?? 'Not configured'}
            />
            <StatCard icon={<Database size={15} />} label="Protocol" value="TCP" color="var(--color-accent-light)" />
          </section>
        )}

        <section className="dashboard-section">
          <SectionTitle
            icon={<Network size={17} />}
            title="Connection"
            subtitle="TRPG TCP 서버 연결 설정과 현재 연결 상태를 확인합니다."
          />
          <div className="connection-grid">
            <ConnectionSettings
              connected={connected}
              connectionInfo={connectionInfo}
              onConnect={onConnect}
              onDisconnect={onDisconnect}
            />
            <ServerStatus connected={connected} connectionInfo={connectionInfo} loading={loading} />
          </div>
        </section>

        <section className="dashboard-section">
          <SectionTitle
            icon={<BarChart3 size={17} />}
            title="Metrics"
            subtitle="실시간 CPU, 메모리, 연결 수 추이를 표시합니다."
          />
          <MetricsPanel serverConnected={connected} />
        </section>

        <section className="dashboard-section">
          <SectionTitle
            icon={<Terminal size={17} />}
            title="Server Logs"
            subtitle="Backend 로그 스트리밍 API가 붙으면 이 영역에서 로그를 확인합니다."
          />
          <LogViewer serverConnected={connected} />
        </section>

        {loading && <LoadingSkeleton />}
      </main>
    </div>
  )
}

function SectionTitle({ icon, title, subtitle }: SectionTitleProps) {
  return (
    <div className="section-title">
      <div className="section-title-row">
        {icon}
        <h2>{title}</h2>
      </div>
      <p>{subtitle}</p>
    </div>
  )
}

function StatCard({ icon, label, value, color = 'var(--color-text-primary)', glow = false }: StatCardProps) {
  return (
    <article
      className="glass-card stat-card"
      style={{ boxShadow: glow ? `0 0 20px rgba(34, 197, 94, 0.16), var(--shadow-card)` : undefined }}
    >
      <div className="flex items-center gap-2">
        {icon}
        <span className="stat-label">{label}</span>
      </div>
      <div className="stat-value" style={{ color }}>
        {value}
      </div>
    </article>
  )
}

function LoadingSkeleton() {
  return (
    <div className="skeleton-stack">
      {[0, 1, 2].map((item) => (
        <div className="glass-card" key={item} style={{ height: 190, padding: 22 }}>
          <div className="skeleton" style={{ width: '60%', height: 16, marginBottom: 12 }} />
          <div className="skeleton" style={{ width: '40%', height: 12, marginBottom: 20 }} />
          <div className="skeleton" style={{ height: 64, marginBottom: 16 }} />
          <div className="flex gap-2">
            <div className="skeleton" style={{ width: 74, height: 30 }} />
            <div className="skeleton" style={{ width: 74, height: 30 }} />
          </div>
        </div>
      ))}
    </div>
  )
}
