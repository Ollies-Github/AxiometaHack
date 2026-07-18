import { MaterialCommunityIcons } from '@expo/vector-icons';
import { StatusBar } from 'expo-status-bar';
import { useEffect, useRef, useState } from 'react';
import {
  Pressable,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from 'react-native';
import { useHat } from '../src/ws/HatContext';

const LS_KEY = 'hatos.modalurl';
function loadUrl(): string {
  try {
    if (typeof localStorage !== 'undefined') return localStorage.getItem(LS_KEY) ?? '';
  } catch {
    /* ignore */
  }
  return '';
}

// Browser media APIs (present on web; typed as any so RN's lib doesn't complain).
const G = globalThis as any;
const mediaSupported = typeof G !== 'undefined' && G.navigator?.mediaDevices && G.MediaRecorder;

export default function Voice() {
  const { status, last, send } = useHat();
  const [modalUrl, setModalUrl] = useState(loadUrl);
  const [recording, setRecording] = useState(false);
  const [busy, setBusy] = useState(false);
  const [script, setScript] = useState('');
  const [lastMs, setLastMs] = useState<number | null>(null);
  const [error, setError] = useState<string | null>(null);

  const recRef = useRef<any>(null);
  const chunksRef = useRef<any[]>([]);
  const streamRef = useRef<any>(null);

  const connected = status === 'connected';

  const saveUrl = (v: string) => {
    setModalUrl(v);
    try {
      if (typeof localStorage !== 'undefined') localStorage.setItem(LS_KEY, v.trim());
    } catch {
      /* ignore */
    }
  };

  const pushToHat = (text: string) => {
    send({ type: 'app.setActive', payload: { id: 'notes' } });
    send({ type: 'notes.script', payload: { text } });
  };

  const startRecording = async () => {
    setError(null);
    if (!mediaSupported) {
      setError('Voice needs the web app in Chrome (mic access).');
      return;
    }
    if (!modalUrl.trim()) {
      setError('Paste your Modal endpoint URL first.');
      return;
    }
    try {
      const stream = await G.navigator.mediaDevices.getUserMedia({ audio: true });
      streamRef.current = stream;
      const rec = new G.MediaRecorder(stream);
      chunksRef.current = [];
      rec.ondataavailable = (e: any) => {
        if (e.data && e.data.size > 0) chunksRef.current.push(e.data);
      };
      rec.onstop = () => transcribe();
      recRef.current = rec;
      rec.start();
      setRecording(true);
      send({ type: 'led', payload: { on: true } });   // light the hat LED while listening
    } catch {
      setError('Could not access the microphone.');
    }
  };

  const stopRecording = () => {
    if (!recording) return;
    setRecording(false);
    try {
      recRef.current?.stop();
      streamRef.current?.getTracks().forEach((t: any) => t.stop());
    } catch {
      /* ignore */
    }
  };

  const transcribe = async () => {
    const blob = new G.Blob(chunksRef.current, { type: 'audio/webm' });
    if (!blob.size) return;
    setBusy(true);
    setError(null);
    const base = modalUrl.trim().replace(/\/+$/, '');
    try {
      const form = new G.FormData();
      form.append('audio', blob, 'audio.webm');
      const t0 = Date.now();
      const res = await fetch(`${base}/transcribe`, { method: 'POST', body: form });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      const text = (data.text ?? '').trim();
      setLastMs(Date.now() - t0);
      if (text) {
        const next = script ? `${script} ${text}` : text;
        setScript(next);
        if (connected) pushToHat(next);
      }
    } catch (e: any) {
      setError(`Transcription failed: ${e?.message ?? 'network error'}`);
    } finally {
      setBusy(false);
      send({ type: 'led', payload: { on: false } });   // done thinking — LED off
    }
  };

  // Hardware push-to-talk: the hat's Port-3 button sends voice start/stop.
  const startRef = useRef(startRecording);
  const stopRef = useRef(stopRecording);
  startRef.current = startRecording;
  stopRef.current = stopRecording;
  useEffect(() => {
    if (!last || last.type !== 'voice') return;
    if (last.payload.action === 'start') startRef.current();
    else stopRef.current();
  }, [last]);

  return (
    <SafeAreaView style={s.safe}>
      <StatusBar style="light" />
      <ScrollView contentContainerStyle={s.container}>
        <Text style={s.eyebrow}>HATOS / VOICE</Text>
        <Text style={s.title}>Dictate</Text>

        {/* Endpoint */}
        <View style={s.card}>
          <Text style={s.label}>Modal endpoint URL</Text>
          <TextInput
            value={modalUrl}
            onChangeText={saveUrl}
            autoCapitalize="none"
            placeholder="https://<you>--hatos-stt-web.modal.run"
            placeholderTextColor="#5b6472"
            style={s.input}
          />
          <Text style={s.hint}>From `modal deploy stt.py`. The app POSTs to /transcribe.</Text>
        </View>

        {/* Mic */}
        <View style={s.micWrap}>
          <Pressable
            onPressIn={startRecording}
            onPressOut={stopRecording}
            disabled={busy}
            style={[s.mic, recording && s.micOn, busy && s.micBusy]}
          >
            <MaterialCommunityIcons
              name={busy ? 'loading' : recording ? 'stop' : 'microphone'}
              size={44}
              color={recording ? '#071008' : '#b8ff65'}
            />
          </Pressable>
          <Text style={s.micLabel}>
            {busy ? 'Transcribing…' : recording ? 'Listening — release to send' : 'Hold to talk'}
          </Text>
          {lastMs != null && !busy && <Text style={s.latency}>round-trip {lastMs} ms</Text>}
          {error && <Text style={s.err}>{error}</Text>}
          {!connected && <Text style={s.warn}>Hat offline — transcript won’t push until connected.</Text>}
        </View>

        {/* Transcript */}
        <Text style={s.section}>Transcript</Text>
        <View style={s.card}>
          <TextInput
            value={script}
            onChangeText={setScript}
            multiline
            textAlignVertical="top"
            placeholder="Your dictated script appears here and on the hat."
            placeholderTextColor="#5b6472"
            style={[s.input, s.scriptInput]}
          />
          <View style={s.row}>
            <Pressable onPress={() => setScript('')} style={s.chip}>
              <Text style={s.chipText}>Clear</Text>
            </Pressable>
            <Pressable
              onPress={() => script.trim() && pushToHat(script)}
              style={[s.btn, s.btnGreen]}
            >
              <Text style={s.btnGreenText}>Push to teleprompter</Text>
            </Pressable>
          </View>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
}

const s = StyleSheet.create({
  safe: { flex: 1, backgroundColor: '#090c12' },
  container: { padding: 18, paddingBottom: 44, gap: 12 },
  eyebrow: { color: '#8a94a8', fontSize: 11, fontWeight: '800', letterSpacing: 1.6, marginTop: 8 },
  title: { color: '#f4f7fb', fontSize: 26, fontWeight: '800', marginBottom: 4 },
  card: { backgroundColor: '#121720', borderColor: '#242c39', borderWidth: 1, borderRadius: 16, padding: 14, gap: 10 },
  label: { color: '#cbd3df', fontSize: 13, fontWeight: '700' },
  input: { backgroundColor: '#05070a', borderColor: '#28303d', borderWidth: 1, borderRadius: 10, color: '#f4f7fb', paddingHorizontal: 12, paddingVertical: 10, fontSize: 14 },
  scriptInput: { minHeight: 120, lineHeight: 20 },
  hint: { color: '#727c90', fontSize: 12, lineHeight: 16 },
  micWrap: { alignItems: 'center', gap: 8, paddingVertical: 14 },
  mic: { width: 104, height: 104, borderRadius: 52, backgroundColor: '#151b25', borderColor: '#b8ff65', borderWidth: 2, alignItems: 'center', justifyContent: 'center' },
  micOn: { backgroundColor: '#b8ff65', borderColor: '#b8ff65' },
  micBusy: { opacity: 0.6 },
  micLabel: { color: '#e9edf4', fontSize: 14, fontWeight: '700' },
  latency: { color: '#78f294', fontSize: 12, fontWeight: '800' },
  err: { color: '#ff7a90', fontSize: 12, textAlign: 'center' },
  warn: { color: '#ffce5c', fontSize: 12, textAlign: 'center' },
  section: { color: '#f4f7fb', fontSize: 15, fontWeight: '800', marginTop: 6 },
  row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', gap: 10 },
  chip: { backgroundColor: '#151b25', borderColor: '#2b3441', borderWidth: 1, borderRadius: 999, paddingHorizontal: 14, paddingVertical: 9 },
  chipText: { color: '#e9edf4', fontWeight: '700', fontSize: 13 },
  btn: { minHeight: 44, borderRadius: 12, alignItems: 'center', justifyContent: 'center', paddingHorizontal: 18, flex: 1 },
  btnGreen: { backgroundColor: '#b8ff65' },
  btnGreenText: { color: '#071008', fontWeight: '900', fontSize: 14 },
});
