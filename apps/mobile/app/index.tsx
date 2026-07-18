import { MaterialCommunityIcons } from '@expo/vector-icons';
import type { Widget, WidgetId } from '@hatos/protocol';
import { Link } from 'expo-router';
import { StatusBar } from 'expo-status-bar';
import { useMemo, useState } from 'react';
import {
  Pressable,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';

const widgets: Widget[] = [
  { id: 'prompt', name: 'AI Prompt', icon: 'creation', color: '#b8ff65', wearerLabel: 'Ask anything', outwardLabel: 'Thinking…' },
  { id: 'speaker', name: 'Speaker Notes', icon: 'microphone-message', color: '#67e8f9', wearerLabel: 'Next: introduce demo', outwardLabel: 'LIVE' },
  { id: 'mood', name: 'Mood Badge', icon: 'emoticon-happy-outline', color: '#ffce5c', wearerLabel: 'Confident', outwardLabel: 'Say hello 👋' },
  { id: 'timer', name: 'Pitch Timer', icon: 'timer-outline', color: '#ff7a90', wearerLabel: '02:00', outwardLabel: 'Demo in progress' },
  { id: 'caption', name: 'Live Captions', icon: 'closed-caption-outline', color: '#a78bfa', wearerLabel: 'Captions ready', outwardLabel: 'Listening…' },
  { id: 'status', name: 'Status', icon: 'access-point', color: '#60a5fa', wearerLabel: 'Hat online', outwardLabel: 'HatOS' },
];

function ScreenPreview({ label, text, accent }: { label: string; text: string; accent: string }) {
  return (
    <View style={styles.previewWrap}>
      <Text style={styles.previewLabel}>{label}</Text>
      <View style={[styles.screen, { borderColor: accent }]}> 
        <View style={[styles.dot, { backgroundColor: accent }]} />
        <Text numberOfLines={2} style={styles.screenText}>{text}</Text>
      </View>
    </View>
  );
}

export default function HomeScreen() {
  const [selected, setSelected] = useState<WidgetId>('prompt');
  const [synced, setSynced] = useState(true);
  const active = useMemo(() => widgets.find((item) => item.id === selected) ?? widgets[0], [selected]);

  const choose = (id: WidgetId) => {
    setSelected(id);
    setSynced(false);
  };

  return (
    <SafeAreaView style={styles.safe}>
      <StatusBar style="light" />
      <ScrollView contentContainerStyle={styles.container}>
        <View style={styles.header}>
          <View>
            <Text style={styles.eyebrow}>HATOS / COMPANION</Text>
            <Text style={styles.title}>Your hat, configured.</Text>
          </View>
          <View style={styles.connection}>
            <View style={styles.onlineDot} />
            <Text style={styles.connectionText}>Demo Hat</Text>
          </View>
        </View>

        <Link href="/control" asChild>
          <Pressable accessibilityRole="button" style={styles.liveButton}>
            <MaterialCommunityIcons name="wifi" size={20} color="#071008" />
            <Text style={styles.liveText}>Live Control (WiFi) →</Text>
          </Pressable>
        </Link>

        <View style={styles.heroCard}>
          <View style={styles.previewRow}>
            <ScreenPreview label="YOU SEE" text={active.wearerLabel} accent={active.color} />
            <ScreenPreview label="THEY SEE" text={active.outwardLabel} accent={active.color} />
          </View>
          <View style={styles.encoderHint}>
            <MaterialCommunityIcons name="rotate-3d-variant" size={18} color="#96a0b5" />
            <Text style={styles.hintText}>Turn encoder to switch · press to activate</Text>
          </View>
        </View>

        <View style={styles.sectionHeading}>
          <Text style={styles.sectionTitle}>Widgets</Text>
          <Text style={styles.sectionMeta}>Choose one for the demo</Text>
        </View>

        <View style={styles.grid}>
          {widgets.map((widget) => {
            const isSelected = widget.id === selected;
            return (
              <Pressable
                accessibilityRole="button"
                key={widget.id}
                onPress={() => choose(widget.id)}
                style={[styles.widget, isSelected && { borderColor: widget.color, backgroundColor: '#1d2430' }]}
              >
                <View style={[styles.iconBox, { backgroundColor: `${widget.color}20` }]}>
                  <MaterialCommunityIcons name={widget.icon as never} size={25} color={widget.color} />
                </View>
                <Text style={styles.widgetName}>{widget.name}</Text>
                <MaterialCommunityIcons
                  name={isSelected ? 'check-circle' : 'plus-circle-outline'}
                  size={21}
                  color={isSelected ? widget.color : '#667085'}
                />
              </Pressable>
            );
          })}
        </View>

        <Pressable onPress={() => setSynced(true)} style={[styles.syncButton, synced && styles.syncedButton]}>
          <MaterialCommunityIcons name={synced ? 'check' : 'upload'} size={20} color="#071008" />
          <Text style={styles.syncText}>{synced ? 'Hat is up to date' : 'Send layout to hat'}</Text>
        </Pressable>

        <Text style={styles.demoNote}>Demo mode is active. Hardware transport plugs into the same sync action.</Text>
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe: { flex: 1, backgroundColor: '#090c12' },
  container: { padding: 20, paddingBottom: 44, gap: 20 },
  header: { flexDirection: 'row', alignItems: 'flex-start', justifyContent: 'space-between', marginTop: 10 },
  eyebrow: { color: '#8a94a8', fontSize: 11, fontWeight: '800', letterSpacing: 1.8 },
  title: { color: '#f4f7fb', fontSize: 27, lineHeight: 33, fontWeight: '800', marginTop: 5 },
  connection: { flexDirection: 'row', alignItems: 'center', gap: 7, backgroundColor: '#151b25', paddingHorizontal: 11, paddingVertical: 8, borderRadius: 999 },
  connectionText: { color: '#cbd3df', fontSize: 12, fontWeight: '700' },
  onlineDot: { width: 7, height: 7, borderRadius: 4, backgroundColor: '#78f294' },
  liveButton: { backgroundColor: '#67e8f9', minHeight: 50, borderRadius: 14, flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 9 },
  liveText: { color: '#071008', fontSize: 15, fontWeight: '900' },
  heroCard: { backgroundColor: '#121720', borderColor: '#242c39', borderWidth: 1, borderRadius: 22, padding: 16, gap: 16 },
  previewRow: { flexDirection: 'row', gap: 12 },
  previewWrap: { flex: 1, gap: 7 },
  previewLabel: { color: '#778196', fontSize: 10, fontWeight: '800', letterSpacing: 1.2 },
  screen: { minHeight: 82, borderRadius: 13, borderWidth: 1, backgroundColor: '#05070a', alignItems: 'center', justifyContent: 'center', padding: 12 },
  screenText: { color: '#f6f8fb', fontSize: 16, fontWeight: '800', textAlign: 'center' },
  dot: { width: 5, height: 5, borderRadius: 3, position: 'absolute', left: 8, top: 8 },
  encoderHint: { borderTopColor: '#28303d', borderTopWidth: 1, paddingTop: 13, flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 7 },
  hintText: { color: '#96a0b5', fontSize: 12 },
  sectionHeading: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'baseline', marginTop: 2 },
  sectionTitle: { color: '#f4f7fb', fontSize: 19, fontWeight: '800' },
  sectionMeta: { color: '#727c90', fontSize: 12 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  widget: { width: '48%', flexGrow: 1, minHeight: 126, backgroundColor: '#121720', borderColor: '#242c39', borderWidth: 1, borderRadius: 17, padding: 14, justifyContent: 'space-between' },
  iconBox: { width: 42, height: 42, borderRadius: 12, alignItems: 'center', justifyContent: 'center' },
  widgetName: { color: '#e9edf4', fontWeight: '700', fontSize: 14 },
  syncButton: { backgroundColor: '#b8ff65', minHeight: 54, borderRadius: 16, flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 9, marginTop: 2 },
  syncedButton: { backgroundColor: '#78f294' },
  syncText: { color: '#071008', fontSize: 15, fontWeight: '900' },
  demoNote: { color: '#667085', fontSize: 11, textAlign: 'center', lineHeight: 16 },
});
