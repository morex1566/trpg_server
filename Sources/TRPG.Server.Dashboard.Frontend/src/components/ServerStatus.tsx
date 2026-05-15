import { AlertTriangle, Play, Square, Zap } from 'lucide-react'
import { useState } from 'react'
import type { ConnectionInfo } from '../api/serverStatus'
import StatusBadge from './StatusBadge'

type ServerStatusProps = {
  connected: boolean
  connectionInfo: ConnectionInfo | null
  loading: boolean
}

type ActionMessage = {
  type: 'success' | 'error'
  text: string
}

export default function ServerStatus({ connected, connectionInfo, loading }: ServerStatusProps) {
  const [actionLoading, setActionLoading] = useState<'start' | 'stop' | null>(null)
  const [actionMessage, setActionMessage] = useState<ActionMessage | null>(null)

  async function handleAction(action: 'start' | 'stop') {
    setActionLoading(action)
    setActionMessage(null)

    try {
      const response = await fetch(`/api/server/${action}`, { method: 'POST' })
      const data = (await response.json()) as { success?: boolean; message?: string; error?: string }

      if (!data.success) {
        throw new Error(data.error ?? `${action} request failed`)
      }

      setActionMessage({ type: 'success', text: data.message ?? `${action} requested` })
    } catch (error) {
      setActionMessage({ type: 'error', text: error instanceof Error ? error.message : `${action} request failed` })
    } finally {
      setActionLoading(null)
    }
  }

  if (loading) {
    return (
      <div className="glass-card server-status-card">
        <div className="skeleton" style={{ width: '50%', height: 16, marginBottom: 12 }} />
        <div className="skeleton" style={{ height: 84, marginBottom: 12 }} />
        <div className="skeleton" style={{ width: '30%', height: 12 }} />
      </div>
    )
  }

  return (
    <div className={`glass-card server-status-card ${connected ? 'connected' : 'disconnected'}`}>
      <div className="card-header">
        <div className="card-heading">
          <div className={`icon-tile ${connected ? 'success' : 'danger'}`}>
            {connected ? <Zap size={19} /> : <AlertTriangle size={19} />}
          </div>
          <div>
            <div className="card-title">TRPG Game Server</div>
            <div className="card-subtitle">
              {connected && connectionInfo ? `${connectionInfo.host}:${connectionInfo.port}` : 'Disconnected'}
            </div>
          </div>
        </div>
        <StatusBadge state={connected ? 'connected' : 'disconnected'} />
      </div>

      {connected ? (
        <>
          <div className="server-actions">
            <button
              className="btn btn-success"
              type="button"
              disabled={actionLoading !== null}
              onClick={() => void handleAction('start')}
            >
              {actionLoading === 'start' ? <span className="spinner" /> : <Play size={15} />}
              Start Server
            </button>
            <button
              className="btn btn-warning"
              type="button"
              disabled={actionLoading !== null}
              onClick={() => void handleAction('stop')}
            >
              {actionLoading === 'stop' ? <span className="spinner" /> : <Square size={15} />}
              Stop Server
            </button>
          </div>

          {actionMessage && <div className={`message-text ${actionMessage.type}`}>{actionMessage.text}</div>}
        </>
      ) : (
        <div className="empty-state" style={{ marginTop: 18 }}>
          서버에 연결되어 있지 않습니다
          <p>TCP Connection 패널에서 서버 연결을 먼저 설정하세요.</p>
        </div>
      )}
    </div>
  )
}
