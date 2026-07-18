import type { Turn } from '@hatos/protocol';
import { StatusBar } from 'expo-status-bar';
import { useState } from 'react';
import {
  Pressable,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from 'react-native';
import { useHatWs } from '../src/ws/useHatWs';

const APPS = ['launcher', 'counter', 'notes', 'music', 'maps'];
const TURNS: Turn[] = ['left', 'slight-left', 'straight', 'slight-right', 'right', 'uturn'];

export default function Control() {
  const { status, last, log, connect, disconnect, send } = useHatWs();
  const [url, setUrl] = useState('ws://192.168.4.1:81');

  // music form
  const [title, setTitle] = useState('Track Title');
  const [artist, setArtist] = useState('Artist Name');
  const [playing, setPlaying] = useState(true);
  const [volume, setVolume] = useState(60);

  // maps form
  const [distance, setDistance] = useState('250');
  const [street, setStreet] = useState('Example Road');

  const connected = status === 'connected';

  return (
    <SafeAreaView style={s.safe}>
      <StatusBar style="light" />
      <ScrollView contentContainerStyle={s.container}>
        <Text style={s.eyebrow}>HATOS / LIVE CONTROL (WiFi)</Text>
        <Text style={s.title}>Hat Control</Text>

        {/* Connection */}
        <View style={s.card}>
          <View style={s.rowBetween}>
            <View style={s.row}>
              <View style={[s.dot, { backgroundColor: connected ? '#78f294' : '#ff7a90' }]} />
              <Text style={s.status}>{status}</Text>
            </View>
            <Pressable
              onPress={() => (connected ? disconnect() : connect(url))}
              style={[s.btn, connected ? s.btnGrey : s.btnGreen]}
            >
              <Text style={connected ? s.btnGreyText : s.btnGreenText}>
                {connected ? 'Disconnect' : 'Connect'}
              </Text>
            </Pressable>
          </View>
          <TextInput value={url} onChangeText={setUrl} autoCapitalize="none" style={s.input} placeholder="ws://192.168.4.1:81" placeholderTextColor="#5b6472" />
          <Text style={s.hint}>Join the WiFi network “HatOS” (password hatos1234), then Connect.</Text>
        </View>

        {/* App switcher */}
        <Text style={s.section}>Active app</Text>
        <View style={s.wrap}>
          {APPS.map((id) => (
            <Pressable key={id} onPress={() => send({ type: 'app.setActive', payload: { id } })} style={s.chip}>
              <Text style={s.chipText}>{id}</Text>
            </Pressable>
          ))}
        </View>

        {/* Music */}
        <Text style={s.section}>Music · now playing</Text>
        <View style={s.card}>
          <TextInput value={title} onChangeText={setTitle} style={s.input} placeholder="title" placeholderTextColor="#5b6472" />
          <TextInput value={artist} onChangeText={setArtist} style={s.input} placeholder="artist" placeholderTextColor="#5b6472" />
          <View style={s.rowBetween}>
            <Pressable onPress={() => setPlaying((p) => !p)} style={[s.chip, playing && s.chipOn]}>
              <Text style={s.chipText}>{playing ? 'Playing' : 'Paused'}</Text>
            </Pressable>
            <View style={s.row}>
              <Pressable onPress={() => setVolume((v) => Math.max(0, v - 10))} style={s.step}><Text style={s.stepText}>–</Text></Pressable>
              <Text style={s.vol}>VOL {volume}</Text>
              <Pressable onPress={() => setVolume((v) => Math.min(100, v + 10))} style={s.step}><Text style={s.stepText}>+</Text></Pressable>
            </View>
          </View>
          <Pressable
            onPress={() => send({ type: 'music.state', payload: { title, artist, positionMs: 42000, durationMs: 210000, isPlaying: playing, volume } })}
            style={[s.btn, s.btnGreen]}
          >
            <Text style={s.btnGreenText}>Push to hat</Text>
          </Pressable>
        </View>

        {/* Maps */}
        <Text style={s.section}>Maps · next maneuver</Text>
        <View style={s.card}>
          <View style={s.wrap}>
            {TURNS.map((t) => (
              <Pressable
                key={t}
                onPress={() => send({ type: 'maps.maneuver', payload: { turn: t, distanceM: parseInt(distance || '0', 10), street } })}
                style={s.chip}
              >
                <Text style={s.chipText}>{t}</Text>
              </Pressable>
            ))}
          </View>
          <View style={s.row}>
            <TextInput value={distance} onChangeText={setDistance} keyboardType="number-pad" style={[s.input, s.flex]} placeholder="distance m" placeholderTextColor="#5b6472" />
            <TextInput value={street} onChangeText={setStreet} style={[s.input, s.flex2]} placeholder="street" placeholderTextColor="#5b6472" />
          </View>
          <Text style={s.hint}>Tap a turn to push it with the distance + street above.</Text>
        </View>

        {/* Incoming */}
        <Text style={s.section}>From hat {last ? `· last: ${last.type}` : ''}</Text>
        <View style={s.card}>
          {log.length === 0 && <Text style={s.hint}>Encoder turns, button presses, and music.command appear here.</Text>}
          {log.map((line, i) => (
            <Text key={i} style={s.logLine} numberOfLines={1}>{line}</Text>
          ))}
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
  row: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  rowBetween: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
  dot: { width: 9, height: 9, borderRadius: 5 },
  status: { color: '#cbd3df', fontSize: 14, fontWeight: '700' },
  input: { backgroundColor: '#05070a', borderColor: '#28303d', borderWidth: 1, borderRadius: 10, color: '#f4f7fb', paddingHorizontal: 12, paddingVertical: 10, fontSize: 14 },
  hint: { color: '#727c90', fontSize: 12, lineHeight: 16 },
  section: { color: '#f4f7fb', fontSize: 15, fontWeight: '800', marginTop: 10 },
  wrap: { flexDirection: 'row', flexWrap: 'wrap', gap: 8 },
  chip: { backgroundColor: '#151b25', borderColor: '#2b3441', borderWidth: 1, borderRadius: 999, paddingHorizontal: 14, paddingVertical: 9 },
  chipOn: { backgroundColor: '#1d2a20', borderColor: '#78f294' },
  chipText: { color: '#e9edf4', fontWeight: '700', fontSize: 13 },
  step: { width: 34, height: 34, borderRadius: 8, backgroundColor: '#151b25', alignItems: 'center', justifyContent: 'center' },
  stepText: { color: '#f4f7fb', fontSize: 18, fontWeight: '800' },
  vol: { color: '#cbd3df', fontWeight: '800', fontSize: 14, minWidth: 66, textAlign: 'center' },
  btn: { minHeight: 44, borderRadius: 12, alignItems: 'center', justifyContent: 'center', paddingHorizontal: 18 },
  btnGreen: { backgroundColor: '#b8ff65' },
  btnGreenText: { color: '#071008', fontWeight: '900', fontSize: 14 },
  btnGrey: { backgroundColor: '#232b37' },
  btnGreyText: { color: '#e9edf4', fontWeight: '800', fontSize: 14 },
  flex: { flex: 1 },
  flex2: { flex: 2 },
  logLine: { color: '#9aa4b5', fontSize: 11, fontFamily: 'monospace' },
});
