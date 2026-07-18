import { MaterialCommunityIcons } from '@expo/vector-icons';
import { Link } from 'expo-router';
import { StatusBar } from 'expo-status-bar';
import { useState } from 'react';
import {
  Pressable,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import { useHat } from '../src/ws/HatContext';

// Per-app presentation, keyed by the id the hat reports in its catalog.
const APP_META: Record<string, { icon: string; color: string; blurb: string }> = {
  launcher: { icon: 'home-variant', color: '#60a5fa', blurb: 'App menu' },
  counter: { icon: 'numeric', color: '#ffce5c', blurb: 'Demo dial' },
  notes: { icon: 'script-text-outline', color: '#b8ff65', blurb: 'Teleprompter' },
  music: { icon: 'music', color: '#67e8f9', blurb: 'Now playing' },
  maps: { icon: 'map-marker-path', color: '#a78bfa', blurb: 'Turn-by-turn' },
};
const FALLBACK_META = { icon: 'application', color: '#8a94a8', blurb: '' };

// Shown before the hat has announced its catalog, so the launcher is never empty.
const FALLBACK_APPS = [
  { id: 'launcher', name: 'Home' },
  { id: 'counter', name: 'Counter' },
  { id: 'notes', name: 'Notes' },
  { id: 'music', name: 'Music' },
  { id: 'maps', name: 'Maps' },
];

export default function HomeScreen() {
  const { status, apps, activeApp, hatName, send, connect } = useHat();
  const [theme, setTheme] = useState<'dark' | 'light'>('dark');
  const connected = status === 'connected';
  const list = apps.length ? apps : FALLBACK_APPS;

  const statusColor = connected ? '#78f294' : status === 'connecting' ? '#ffce5c' : '#ff7a90';
  const statusText = connected ? (hatName ?? 'Connected') : status === 'connecting' ? 'Connecting…' : 'Offline';

  return (
    <SafeAreaView style={styles.safe}>
      <StatusBar style="light" />
      <ScrollView contentContainerStyle={styles.container}>
        <View style={styles.header}>
          <View style={styles.flex1}>
            <Text style={styles.eyebrow}>HATOS / COMPANION</Text>
            <Text style={styles.title}>Your hat</Text>
          </View>
          <View style={styles.connection}>
            <View style={[styles.dot, { backgroundColor: statusColor }]} />
            <Text style={styles.connectionText}>{statusText}</Text>
          </View>
        </View>

        {!connected && (
          <Pressable onPress={() => connect()} style={styles.reconnect}>
            <MaterialCommunityIcons name="wifi-sync" size={18} color="#071008" />
            <Text style={styles.reconnectText}>
              {status === 'connecting' ? 'Connecting…' : 'Reconnect'}
            </Text>
          </Pressable>
        )}

        <View style={styles.actionRow}>
          <Link href="/voice" asChild>
            <Pressable accessibilityRole="button" style={StyleSheet.flatten([styles.actionBtn, styles.voiceBtn])}>
              <MaterialCommunityIcons name="microphone" size={19} color="#071008" />
              <Text style={styles.actionText}>Voice</Text>
            </Pressable>
          </Link>
          <Link href="/control" asChild>
            <Pressable accessibilityRole="button" style={StyleSheet.flatten([styles.actionBtn, styles.controlBtn])}>
              <MaterialCommunityIcons name="tune-vertical" size={19} color="#071008" />
              <Text style={styles.actionText}>Control</Text>
            </Pressable>
          </Link>
        </View>

        <View style={styles.themeRow}>
          <Text style={styles.themeLabel}>Screen theme</Text>
          <View style={styles.segment}>
            {(['dark', 'light'] as const).map((m) => (
              <Pressable
                key={m}
                onPress={() => {
                  setTheme(m);
                  send({ type: 'theme', payload: { mode: m } });
                }}
                style={[styles.segBtn, theme === m && styles.segBtnOn]}
              >
                <Text style={[styles.segText, theme === m && styles.segTextOn]}>{m}</Text>
              </Pressable>
            ))}
          </View>
        </View>

        <View style={styles.sectionHeading}>
          <Text style={styles.sectionTitle}>Apps</Text>
          <Text style={styles.sectionMeta}>
            {connected ? 'Tap to launch on the hat' : 'Connect to launch'}
          </Text>
        </View>

        <View style={styles.grid}>
          {list.map((app) => {
            const meta = APP_META[app.id] ?? FALLBACK_META;
            const isActive = activeApp === app.id;
            return (
              <Pressable
                accessibilityRole="button"
                key={app.id}
                disabled={!connected}
                onPress={() => send({ type: 'app.setActive', payload: { id: app.id } })}
                style={[
                  styles.widget,
                  !connected && styles.widgetDim,
                  isActive && { borderColor: meta.color, backgroundColor: '#1d2430' },
                ]}
              >
                <View style={styles.widgetTop}>
                  <View style={[styles.iconBox, { backgroundColor: `${meta.color}20` }]}>
                    <MaterialCommunityIcons name={meta.icon as never} size={24} color={meta.color} />
                  </View>
                  {isActive && (
                    <View style={[styles.badge, { backgroundColor: meta.color }]}>
                      <Text style={styles.badgeText}>ON SCREEN</Text>
                    </View>
                  )}
                </View>
                <Text style={styles.widgetName}>{app.name}</Text>
                <Text style={styles.widgetBlurb}>{meta.blurb}</Text>
              </Pressable>
            );
          })}
        </View>

        <Text style={styles.demoNote}>
          The launcher mirrors the hat’s real apps. Turn the encoder on the hat to switch too —
          it stays in sync here.
        </Text>
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe: { flex: 1, backgroundColor: '#090c12' },
  container: { padding: 20, paddingBottom: 44, gap: 16 },
  flex1: { flex: 1 },
  header: { flexDirection: 'row', alignItems: 'flex-start', justifyContent: 'space-between', marginTop: 10 },
  eyebrow: { color: '#8a94a8', fontSize: 11, fontWeight: '800', letterSpacing: 1.8 },
  title: { color: '#f4f7fb', fontSize: 27, lineHeight: 33, fontWeight: '800', marginTop: 5 },
  connection: { flexDirection: 'row', alignItems: 'center', gap: 7, backgroundColor: '#151b25', paddingHorizontal: 11, paddingVertical: 8, borderRadius: 999 },
  connectionText: { color: '#cbd3df', fontSize: 12, fontWeight: '700' },
  dot: { width: 8, height: 8, borderRadius: 4 },
  reconnect: { backgroundColor: '#ffce5c', minHeight: 44, borderRadius: 12, flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 8 },
  reconnectText: { color: '#071008', fontSize: 14, fontWeight: '900' },
  actionRow: { flexDirection: 'row', gap: 10 },
  actionBtn: { flex: 1, minHeight: 50, borderRadius: 14, flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 8 },
  voiceBtn: { backgroundColor: '#b8ff65' },
  controlBtn: { backgroundColor: '#67e8f9' },
  actionText: { color: '#071008', fontSize: 15, fontWeight: '900' },
  themeRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', backgroundColor: '#121720', borderColor: '#242c39', borderWidth: 1, borderRadius: 14, paddingVertical: 8, paddingHorizontal: 14 },
  themeLabel: { color: '#cbd3df', fontSize: 14, fontWeight: '700' },
  segment: { flexDirection: 'row', backgroundColor: '#05070a', borderRadius: 999, padding: 3, gap: 2 },
  segBtn: { paddingHorizontal: 16, paddingVertical: 7, borderRadius: 999 },
  segBtnOn: { backgroundColor: '#b8ff65' },
  segText: { color: '#8a94a8', fontSize: 12, fontWeight: '800', textTransform: 'capitalize' },
  segTextOn: { color: '#071008' },
  sectionHeading: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'baseline', marginTop: 4 },
  sectionTitle: { color: '#f4f7fb', fontSize: 19, fontWeight: '800' },
  sectionMeta: { color: '#727c90', fontSize: 12 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  widget: { width: '48%', flexGrow: 1, minHeight: 118, backgroundColor: '#121720', borderColor: '#242c39', borderWidth: 1, borderRadius: 17, padding: 14, justifyContent: 'space-between', gap: 6 },
  widgetDim: { opacity: 0.55 },
  widgetTop: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
  iconBox: { width: 42, height: 42, borderRadius: 12, alignItems: 'center', justifyContent: 'center' },
  badge: { borderRadius: 999, paddingHorizontal: 8, paddingVertical: 3 },
  badgeText: { color: '#071008', fontSize: 9, fontWeight: '900', letterSpacing: 0.5 },
  widgetName: { color: '#e9edf4', fontWeight: '800', fontSize: 15 },
  widgetBlurb: { color: '#727c90', fontSize: 12 },
  demoNote: { color: '#667085', fontSize: 11, textAlign: 'center', lineHeight: 16, marginTop: 4 },
});
