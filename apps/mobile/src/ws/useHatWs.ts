import { useCallback, useEffect, useRef, useState } from 'react';
import type { PhoneToHat, HatToPhone } from '@hatos/protocol';

export type WsStatus = 'disconnected' | 'connecting' | 'connected';

// Talks to the HatOS device's WebSocket server (SoftAP default 192.168.4.1:81).
// Works in plain Expo Go — WebSocket is a built-in, no native module needed.
export function useHatWs() {
  const wsRef = useRef<WebSocket | null>(null);
  const [status, setStatus] = useState<WsStatus>('disconnected');
  const [last, setLast] = useState<HatToPhone | null>(null);
  const [log, setLog] = useState<string[]>([]);

  const connect = useCallback((url: string) => {
    wsRef.current?.close();
    setStatus('connecting');
    const ws = new WebSocket(url);
    ws.onopen = () => setStatus('connected');
    ws.onclose = () => setStatus('disconnected');
    ws.onerror = () => setStatus('disconnected');
    ws.onmessage = (e: WebSocketMessageEvent) => {
      try {
        const m = JSON.parse(String(e.data)) as HatToPhone;
        setLast(m);
        setLog((l) => [JSON.stringify(m), ...l].slice(0, 25));
      } catch {
        // ignore non-JSON frames
      }
    };
    wsRef.current = ws;
  }, []);

  const disconnect = useCallback(() => {
    wsRef.current?.close();
    wsRef.current = null;
    setStatus('disconnected');
  }, []);

  const send = useCallback((m: PhoneToHat) => {
    const ws = wsRef.current;
    if (ws && ws.readyState === 1) ws.send(JSON.stringify(m));
  }, []);

  useEffect(() => () => wsRef.current?.close(), []);

  return { status, last, log, connect, disconnect, send };
}
