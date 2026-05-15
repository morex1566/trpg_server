import { ArrowDown, Play, Square, Trash2 } from 'lucide-react'
import { useCallback, useEffect, useRef, useState } from 'react'
import StatusBadge from './StatusBadge'

type LogViewerProps = {
  serverConnected: boolean
}

type ServerLog = {
  type: 'log' | 'info' | 'warning' | 'error'
  text: string
  timestamp: string
}

type LogsResponse = {
  logs?: Array<string | { type?: string; message?: string; timestamp?: string }>
}

export default function LogViewer({ serverConnected }: LogViewerProps) {
  const [lines, setLines] = useState<ServerLog[]>([])
  const [streaming, setStreaming] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [autoScroll, setAutoScroll] = useState(true)
  const bottomRef = useRef<HTMLDivElement | null>(null)
  const intervalRef = useRef<number | null>(null)

  const fetchLogs = useCallback(async () => {
    try {
      const response = await fetch('/api/server/logs?lines=100')

      if (!response.ok) {
        throw new Error('Logs API is not available')
      }

      const data = (await response.json()) as LogsResponse
      const nextLines = (data.logs ?? []).map((item): ServerLog => {
        if (typeof item === 'string') {
          return { type: 'log', text: item, timestamp: now() }
        }

        return {
          type: normalizeLogType(item.type),
          text: item.message ?? JSON.stringify(item),
          timestamp: item.timestamp ?? now(),
        }
      })

      setLines(nextLines)
      setError(null)
    } catch (caught) {
      setError(caught instanceof Error ? caught.message : 'Logs request failed')
    }
  }, [])

  const stopStreaming = useCallback(() => {
    setStreaming(false)

    if (intervalRef.current !== null) {
      window.clearInterval(intervalRef.current)
      intervalRef.current = null
    }
  }, [])

  const startStreaming = useCallback(() => {
    setStreaming(true)
    void fetchLogs()
    intervalRef.current = window.setInterval(() => {
      void fetchLogs()
    }, 2000)
  }, [fetchLogs])

  useEffect(() => stopStreaming, [stopStreaming])

  useEffect(() => {
    if (autoScroll) {
      bottomRef.current?.scrollIntoView({ behavior: 'smooth' })
    }
  }, [autoScroll, lines])

  return (
    <div className="glass-card" style={{ padding: '20px 22px' }}>
      <div className="card-header" style={{ marginBottom: 14 }}>
        <div>
          <div className="card-title">Server Logs</div>
          <div className="card-subtitle">HTTP polling, 2s interval</div>
        </div>

        <div className="flex items-center gap-2">
          <StatusBadge state={streaming ? 'running' : 'paused'} size="sm" />
          <button
            className={`btn btn-sm ${autoScroll ? 'btn-primary' : 'btn-ghost'}`}
            type="button"
            title="자동 스크롤"
            onClick={() => setAutoScroll((value) => !value)}
          >
            <ArrowDown size={13} />
          </button>
          <button className="btn btn-ghost btn-sm" type="button" onClick={() => setLines([])}>
            <Trash2 size={13} />
            Clear
          </button>
          {streaming ? (
            <button className="btn btn-danger btn-sm" type="button" onClick={stopStreaming}>
              <Square size={13} />
              Stop
            </button>
          ) : (
            <button className="btn btn-success btn-sm" type="button" disabled={!serverConnected} onClick={startStreaming}>
              <Play size={13} />
              Start
            </button>
          )}
        </div>
      </div>

      {error && <div className="message-text error" style={{ marginBottom: 10 }}>{error}</div>}

      <div
        className="terminal"
        onScroll={(event) => {
          const target = event.currentTarget
          setAutoScroll(target.scrollHeight - target.scrollTop - target.clientHeight < 30)
        }}
      >
        {lines.length === 0 ? (
          <div className="terminal-empty">
            {streaming
              ? '로그 대기 중...'
              : serverConnected
                ? 'Start 버튼을 눌러 서버 로그를 확인하세요.'
                : '서버에 연결되면 로그를 확인할 수 있습니다.'}
          </div>
        ) : (
          lines.map((line, index) => <LogLine key={`${line.timestamp}-${index}`} line={line} />)
        )}
        <div ref={bottomRef} />
      </div>

      <div className="card-subtitle" style={{ marginTop: 8, textAlign: 'right' }}>
        {lines.length} lines
      </div>
    </div>
  )
}

function LogLine({ line }: { line: ServerLog }) {
  const lowerText = line.text.toLowerCase()
  const color = line.type === 'error' || lowerText.includes('error') || lowerText.includes('failed')
    ? 'var(--color-stopped)'
    : line.type === 'warning' || lowerText.includes('warn')
      ? 'var(--color-warning)'
      : line.type === 'info' || lowerText.includes('success')
        ? 'var(--color-running)'
        : '#a8b1c5'

  return (
    <div className="log-line" style={{ color }}>
      <span className="log-time">{line.timestamp}</span>
      <span className="log-text">{line.text.trimEnd()}</span>
    </div>
  )
}

function normalizeLogType(type: string | undefined): ServerLog['type'] {
  if (type === 'info' || type === 'warning' || type === 'error') {
    return type
  }

  return 'log'
}

function now() {
  return new Date().toLocaleTimeString('ko-KR', { hour12: false })
}
