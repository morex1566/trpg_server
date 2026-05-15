export type ConnectionInfo = {
  host: string
  port: number
  label: string
}

export type ServerStatus = {
  connected: boolean
  connectionInfo: ConnectionInfo | null
  activeConnectionCount: number
  pendingPacketCount: number
  ramPercentage: number
  uptimeMs: number
}

type ServerStatusResponse = {
  connected?: boolean
  serverHost?: string
  serverPort?: number
  label?: string
  activeConnectionCount?: number
  pendingPacketCount?: number
  ramPercentage?: number
  uptimeMs?: number
}

export async function fetchServerStatus(): Promise<ServerStatus> {
  const response = await fetch('/api/server/status')

  if (!response.ok) {
    throw new Error(`Failed to fetch server status: ${response.status}`)
  }

  const data = (await response.json()) as ServerStatusResponse
  const connected = data.connected ?? false
  const host = data.serverHost ?? '127.0.0.1'
  const port = data.serverPort ?? 60000

  return {
    connected,
    connectionInfo: connected
      ? {
          host,
          port,
          label: data.label ?? `${host}:${port}`,
        }
      : null,
    activeConnectionCount: data.activeConnectionCount ?? 0,
    pendingPacketCount: data.pendingPacketCount ?? 0,
    ramPercentage: data.ramPercentage ?? 0,
    uptimeMs: data.uptimeMs ?? 0,
  }
}
