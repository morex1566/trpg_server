import { useCallback, useEffect, useState } from 'react'
import { fetchServerStatus, type ConnectionInfo } from './api/serverStatus'
import Dashboard from './components/Dashboard'
import './App.css'

function App() {
  const [connected, setConnected] = useState(false)
  const [connectionInfo, setConnectionInfo] = useState<ConnectionInfo | null>(null)
  const [loading, setLoading] = useState(true)
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null)

  const refreshStatus = useCallback(async () => {
    try {
      const status = await fetchServerStatus()
      setConnected(status.connected)
      setConnectionInfo(status.connectionInfo)
      setLastUpdated(new Date())
    } catch {
      setConnected(false)
      setConnectionInfo(null)
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    window.setTimeout(() => {
      void refreshStatus()
    }, 0)

    const interval = window.setInterval(() => {
      void refreshStatus()
    }, 5000)

    return () => window.clearInterval(interval)
  }, [refreshStatus])

  return (
    <Dashboard
      connected={connected}
      connectionInfo={connectionInfo}
      loading={loading}
      lastUpdated={lastUpdated}
      onRefresh={refreshStatus}
      onConnect={refreshStatus}
      onDisconnect={refreshStatus}
    />
  )
}

export default App
