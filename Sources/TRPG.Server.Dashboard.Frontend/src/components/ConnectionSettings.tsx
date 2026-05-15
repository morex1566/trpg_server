import { ChevronDown, Link2, Plug, Unplug } from 'lucide-react'
import { useEffect, useState } from 'react'
import type { ConnectionInfo } from '../api/serverStatus'

type ConnectionSettingsProps = {
  connected: boolean
  connectionInfo: ConnectionInfo | null
  onConnect: () => void | Promise<void>
  onDisconnect: () => void | Promise<void>
}

type Message = {
  type: 'success' | 'error'
  text: string
}

const presets = [
  { label: 'Localhost', host: '127.0.0.1', port: '60000' },
  { label: 'Development', host: '127.0.0.1', port: '60000' },
]

export default function ConnectionSettings({
  connected,
  connectionInfo,
  onConnect,
  onDisconnect,
}: ConnectionSettingsProps) {
  const [host, setHost] = useState(connectionInfo?.host ?? '127.0.0.1')
  const [port, setPort] = useState((connectionInfo?.port ?? 60000).toString())
  const [saving, setSaving] = useState(false)
  const [message, setMessage] = useState<Message | null>(null)
  const [expanded, setExpanded] = useState(!connected)

  useEffect(() => {
    if (connectionInfo) {
      window.queueMicrotask(() => {
        setHost(connectionInfo.host)
        setPort(connectionInfo.port.toString())
      })
    }
  }, [connectionInfo])

  async function handleConnect() {
    if (!host.trim() || !port.trim()) return

    setSaving(true)
    setMessage(null)

    try {
      const response = await fetch('/api/server/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ host: host.trim(), port: Number.parseInt(port.trim(), 10) }),
      })
      const data = (await response.json()) as { success?: boolean; message?: string; error?: string }

      if (!data.success) {
        throw new Error(data.error ?? 'Connection failed')
      }

      setMessage({ type: 'success', text: data.message ?? 'Connected' })
      setExpanded(false)
      await onConnect()
    } catch (error) {
      setMessage({ type: 'error', text: error instanceof Error ? error.message : 'Connection failed' })
    } finally {
      setSaving(false)
    }
  }

  async function handleDisconnect() {
    try {
      await fetch('/api/server/disconnect', { method: 'POST' })
      setExpanded(true)
      await onDisconnect()
    } catch (error) {
      setMessage({ type: 'error', text: error instanceof Error ? error.message : 'Disconnect failed' })
    }
  }

  return (
    <div className="glass-card" style={{ padding: '20px 22px' }}>
      <div className="card-header" role="button" tabIndex={0} onClick={() => setExpanded((value) => !value)}>
        <div className="card-heading">
          <div className={`icon-tile ${connected ? 'success' : ''}`}>{connected ? <Link2 size={17} /> : <Plug size={17} />}</div>
          <div>
            <div className="card-title">TCP Connection</div>
            <div className="card-subtitle">
              {connected && connectionInfo ? `${connectionInfo.host}:${connectionInfo.port}` : 'Disconnected'}
            </div>
          </div>
        </div>

        <div className="flex items-center gap-3">
          {connected && (
            <button
              className="btn btn-danger btn-sm"
              type="button"
              onClick={(event) => {
                event.stopPropagation()
                void handleDisconnect()
              }}
            >
              <Unplug size={13} />
              Disconnect
            </button>
          )}
          <ChevronDown size={15} style={{ transform: expanded ? 'rotate(180deg)' : 'rotate(0deg)' }} />
        </div>
      </div>

      {expanded && (
        <div className="collapsible-body">
          <div style={{ marginBottom: 14 }}>
            <div className="field-label">Presets</div>
            <div className="flex gap-2">
              {presets.map((preset) => (
                <button
                  className={`btn btn-sm ${host === preset.host && port === preset.port ? 'btn-primary' : 'btn-ghost'}`}
                  key={preset.label}
                  type="button"
                  onClick={() => {
                    setHost(preset.host)
                    setPort(preset.port)
                  }}
                >
                  {preset.label}
                </button>
              ))}
            </div>
          </div>

          <div className="connection-form-grid">
            <div>
              <label className="field-label" htmlFor="input-server-host">
                Host
              </label>
              <input
                className="field-input"
                id="input-server-host"
                type="text"
                value={host}
                onChange={(event) => setHost(event.target.value)}
                placeholder="127.0.0.1"
              />
            </div>
            <div>
              <label className="field-label" htmlFor="input-server-port">
                Port
              </label>
              <input
                className="field-input"
                id="input-server-port"
                type="text"
                value={port}
                onChange={(event) => setPort(event.target.value)}
                placeholder="60000"
              />
            </div>
          </div>

          <div className="flex items-center gap-3">
            <button
              className="btn btn-primary"
              type="button"
              disabled={saving || !host.trim() || !port.trim()}
              onClick={() => void handleConnect()}
            >
              {saving ? <span className="spinner" /> : <Plug size={14} />}
              Save & Connect
            </button>
            {message && <span className={`message-text ${message.type}`}>{message.text}</span>}
          </div>
        </div>
      )}
    </div>
  )
}
