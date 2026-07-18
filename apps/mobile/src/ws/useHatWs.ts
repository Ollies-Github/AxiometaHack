import { useCallback, useEffect, useRef, useState } from 'react';
import type { PhoneToHat, HatToPhone, AppInfo } from '@hatos/protocol';

export type WsStatus = 'disconnected' | 'connecting' | 'connected';

const DEFAULT_URL = 'ws://172.20.10.2:81';
const LS_KEY = 'hatos.wsurl';
const MODAL_KEY = 'hatos.modalurl';
const RETRY_MS = 1800;

function loadModalUrl(): string {
  try {
    if (typeof localStorage !== 'undefined') return (localStorage.getItem(MODAL_KEY) ?? '').trim();
  } catch {
    /* ignore */
  }
  return '';
}

function loadUrl(): string {
  try {
    if (typeof localStorage !== 'undefined') {
      const v = localStorage.getItem(LS_KEY);
      if (v) return v;
    }
  } catch {
    /* localStorage unavailable (native) — fall through to default */
  }
  return DEFAULT_URL;
}
function persistUrl(v: string) {
  try {
    if (typeof localStorage !== 'undefined') localStorage.setItem(LS_KEY, v);
  } catch {
    /* ignore */
  }
}

// Single WebSocket connection to the hat, with auto-connect on mount,
// auto-reconnect on drop, a persisted URL, and live tracking of the hat's app
// catalog (from hat.ready) and which app is on screen (from app.changed).
// Meant to be instantiated once, at the app root, and shared via HatContext.
export function useHatWs() {
  const [url, setUrlState] = useState<string>(loadUrl);
  const [status, setStatus] = useState<WsStatus>('disconnected');
  const [last, setLast] = useState<HatToPhone | null>(null);
  const [log, setLog] = useState<string[]>([]);
  const [apps, setApps] = useState<AppInfo[]>([]);
  const [activeApp, setActiveApp] = useState<string | null>(null);
  const [hatName, setHatName] = useState<string | null>(null);

  const wsRef = useRef<WebSocket | null>(null);
  const urlRef = useRef(url);
  const manualClose = useRef(false);
  const retryTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const connectRef = useRef<(target?: string) => void>(() => {});

  const clearRetry = () => {
    if (retryTimer.current) {
      clearTimeout(retryTimer.current);
      retryTimer.current = null;
    }
  };

  // A binary WS frame from the hat is a captured WAV (onboard mic). POST it to
  // the Modal STT endpoint, then push the transcript to the hat teleprompter and
  // turn the AI-button LED off.
  const handleWav = async (buf: ArrayBuffer) => {
    const sendJson = (o: PhoneToHat) => {
      const ws = wsRef.current;
      if (ws && ws.readyState === 1) ws.send(JSON.stringify(o));
    };
    const url = loadModalUrl();
    if (!url) {
      setLog((l) => ['[voice] no Modal URL set (Voice screen)', ...l].slice(0, 30));
      sendJson({ type: 'led', payload: { on: false } });
      return;
    }
    try {
      const G = globalThis as any;
      const form = new G.FormData();
      form.append('audio', new G.Blob([buf], { type: 'audio/wav' }), 'a.wav');
      const res = await fetch(`${url.replace(/\/+$/, '')}/transcribe`, { method: 'POST', body: form });
      const j = await res.json();
      const text = String(j.text ?? '').trim();
      if (text) {
        // The transcript becomes the teleprompter script.
        sendJson({ type: 'app.setActive', payload: { id: 'notes' } });
        sendJson({ type: 'notes.script', payload: { text } });
      }
      setLog((l) => [`[voice] "${text}" (${j.ms ?? '?'}ms)`, ...l].slice(0, 30));
    } catch (e: any) {
      setLog((l) => [`[voice] transcribe failed: ${e?.message ?? 'error'}`, ...l].slice(0, 30));
    } finally {
      sendJson({ type: 'led', payload: { on: false } });
    }
  };

  const connect = useCallback((target?: string) => {
    const u = target ?? urlRef.current;
    urlRef.current = u;
    manualClose.current = false;
    clearRetry();
    wsRef.current?.close();
    setStatus('connecting');

    let ws: WebSocket;
    try {
      ws = new WebSocket(u);
    } catch {
      setStatus('disconnected');
      retryTimer.current = setTimeout(() => connectRef.current(), RETRY_MS);
      return;
    }
    wsRef.current = ws;
    try { (ws as any).binaryType = 'arraybuffer'; } catch { /* native default is fine */ }

    ws.onopen = () => {
      if (wsRef.current !== ws) return; // superseded by a newer socket
      setStatus('connected');
    };
    ws.onclose = () => {
      if (wsRef.current !== ws) return; // stale socket (e.g. StrictMode remount)
      setStatus('disconnected');
      if (!manualClose.current) {
        clearRetry();
        retryTimer.current = setTimeout(() => connectRef.current(), RETRY_MS);
      }
    };
    ws.onerror = () => {
      /* onclose fires next and handles the retry */
    };
    ws.onmessage = (e: WebSocketMessageEvent) => {
      if (wsRef.current !== ws) return;
      const data: any = (e as any).data;
      if (data instanceof ArrayBuffer) { handleWav(data); return; }   // captured WAV
      let m: HatToPhone;
      try {
        m = JSON.parse(String(data)) as HatToPhone;
      } catch {
        return; // ignore non-JSON frames
      }
      setLast(m);
      setLog((l) => [JSON.stringify(m), ...l].slice(0, 30));
      if (m.type === 'hat.ready') {
        setApps(m.payload.apps);
        setHatName(m.payload.name);
      } else if (m.type === 'app.changed') {
        setActiveApp(m.payload.id);
      }
    };
  }, []);
  connectRef.current = connect;

  const disconnect = useCallback(() => {
    manualClose.current = true;
    clearRetry();
    wsRef.current?.close();
    wsRef.current = null;
    setStatus('disconnected');
  }, []);

  const setUrl = useCallback((v: string) => {
    setUrlState(v);
    urlRef.current = v;
    persistUrl(v);
  }, []);

  const send = useCallback((m: PhoneToHat) => {
    const ws = wsRef.current;
    if (ws && ws.readyState === 1) {
      ws.send(JSON.stringify(m)); // JSON.stringify is already compact (no spaces)
      return true;
    }
    return false;
  }, []);

  // Auto-connect once on mount; tear the socket down on unmount.
  useEffect(() => {
    connect(urlRef.current);
    return () => {
      manualClose.current = true;
      clearRetry();
      wsRef.current?.close();
    };
  }, [connect]);

  return { url, setUrl, status, last, log, apps, activeApp, hatName, connect, disconnect, send };
}
