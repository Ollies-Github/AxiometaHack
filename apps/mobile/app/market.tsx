import { MaterialCommunityIcons } from '@expo/vector-icons';
import { StatusBar } from 'expo-status-bar';
import { useMemo, useState } from 'react';
import {
  Pressable,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from 'react-native';

type MarketApp = {
  id: string;
  name: string;
  blurb: string;
  category: string;
  icon: string;
  color: string;
  core?: boolean; // ships with HatOS
};

// Mock catalogue for the marketplace FE. The first four are the real on-device
// apps (already installed); the rest are browsable concepts.
const CATALOG: MarketApp[] = [
  { id: 'music', name: 'Music', blurb: 'Spotify now-playing + knob remote', category: 'Music', icon: 'music', color: '#67e8f9', core: true },
  { id: 'maps', name: 'Maps', blurb: 'Turn-by-turn navigation arrows', category: 'Navigation', icon: 'map-marker-path', color: '#a78bfa', core: true },
  { id: 'notes', name: 'Teleprompter', blurb: 'Read notes, dictate by voice', category: 'Productivity', icon: 'script-text-outline', color: '#b8ff65', core: true },
  { id: 'counter', name: 'Counter', blurb: 'A simple dial counter', category: 'Utilities', icon: 'numeric', color: '#ffce5c', core: true },
  { id: 'pomodoro', name: 'Pomodoro', blurb: 'Focus timer with breaks', category: 'Productivity', icon: 'timer-outline', color: '#ff7a90' },
  { id: 'compass', name: 'Compass', blurb: 'Live heading + bearing', category: 'Navigation', icon: 'compass-outline', color: '#60a5fa' },
  { id: 'weather', name: 'Weather', blurb: 'Now + hourly forecast', category: 'Utilities', icon: 'weather-partly-cloudy', color: '#67e8f9' },
  { id: 'clock', name: 'World Clock', blurb: 'Time across zones', category: 'Utilities', icon: 'clock-outline', color: '#a78bfa' },
  { id: 'steps', name: 'Steps', blurb: 'Daily step counter', category: 'Fitness', icon: 'shoe-print', color: '#78f294' },
  { id: 'heart', name: 'Heart Rate', blurb: 'Live BPM from your phone', category: 'Fitness', icon: 'heart-pulse', color: '#ff7a90' },
  { id: 'metronome', name: 'Metronome', blurb: 'Tempo + tap BPM', category: 'Music', icon: 'metronome', color: '#ffce5c' },
  { id: 'level', name: 'Bubble Level', blurb: 'Spirit level + angle', category: 'Tools', icon: 'spirit-level', color: '#b8ff65' },
  { id: 'dice', name: 'Dice', blurb: 'Roll with a shake', category: 'Fun', icon: 'dice-multiple', color: '#67e8f9' },
  { id: 'translate', name: 'Translator', blurb: 'Speak to translate live', category: 'Productivity', icon: 'translate', color: '#a78bfa' },
  { id: 'notify', name: 'Notifications', blurb: 'Phone alerts on your hat', category: 'Utilities', icon: 'bell-outline', color: '#ffce5c' },
  { id: 'stocks', name: 'Stocks', blurb: 'Tickers + sparklines', category: 'Utilities', icon: 'chart-line', color: '#78f294' },
];

const CATEGORIES = ['All', 'Productivity', 'Navigation', 'Music', 'Utilities', 'Fitness', 'Tools', 'Fun'];

const FEATURED = CATALOG.find((a) => a.id === 'pomodoro')!;

export default function Market() {
  const [installed, setInstalled] = useState<Set<string>>(
    () => new Set(CATALOG.filter((a) => a.core).map((a) => a.id)),
  );
  const [cat, setCat] = useState('All');
  const [query, setQuery] = useState('');

  const toggle = (id: string) =>
    setInstalled((prev) => {
      const next = new Set(prev);
      next.has(id) ? next.delete(id) : next.add(id);
      return next;
    });

  const list = useMemo(() => {
    const q = query.trim().toLowerCase();
    return CATALOG.filter(
      (a) =>
        (cat === 'All' || a.category === cat) &&
        (!q || a.name.toLowerCase().includes(q) || a.blurb.toLowerCase().includes(q)),
    );
  }, [cat, query]);

  const featuredOn = installed.has(FEATURED.id);

  return (
    <SafeAreaView style={s.safe}>
      <StatusBar style="light" />
      <ScrollView contentContainerStyle={s.container}>
        <Text style={s.eyebrow}>HATOS / MARKET</Text>
        <View style={s.titleRow}>
          <Text style={s.title}>App Market</Text>
          <View style={s.countPill}>
            <MaterialCommunityIcons name="hat-fedora" size={13} color="#78f294" />
            <Text style={s.countText}>{installed.size} on hat</Text>
          </View>
        </View>

        {/* Search */}
        <View style={s.search}>
          <MaterialCommunityIcons name="magnify" size={18} color="#5b6472" />
          <TextInput
            value={query}
            onChangeText={setQuery}
            placeholder="Search apps"
            placeholderTextColor="#5b6472"
            style={s.searchInput}
            autoCapitalize="none"
          />
        </View>

        {/* Categories */}
        <ScrollView horizontal showsHorizontalScrollIndicator={false} contentContainerStyle={s.chips}>
          {CATEGORIES.map((c) => (
            <Pressable key={c} onPress={() => setCat(c)} style={[s.chip, cat === c && s.chipOn]}>
              <Text style={[s.chipText, cat === c && s.chipTextOn]}>{c}</Text>
            </Pressable>
          ))}
        </ScrollView>

        {/* Featured */}
        {(cat === 'All' || cat === FEATURED.category) && !query && (
          <View style={s.featured}>
            <View style={s.featuredTop}>
              <Text style={s.featuredTag}>FEATURED</Text>
              <View style={[s.featIcon, { backgroundColor: `${FEATURED.color}22` }]}>
                <MaterialCommunityIcons name={FEATURED.icon as never} size={30} color={FEATURED.color} />
              </View>
            </View>
            <Text style={s.featuredName}>{FEATURED.name}</Text>
            <Text style={s.featuredBlurb}>Stay on pace during your pitch — focus sprints with buzz-free break reminders, right on your hat.</Text>
            <Pressable onPress={() => toggle(FEATURED.id)} style={[s.getBtn, featuredOn ? s.getBtnDone : s.getBtnOn]}>
              <MaterialCommunityIcons name={featuredOn ? 'check' : 'plus'} size={17} color={featuredOn ? '#78f294' : '#071008'} />
              <Text style={[s.getText, featuredOn && s.getTextDone]}>{featuredOn ? 'Added to hat' : 'Add to hat'}</Text>
            </Pressable>
          </View>
        )}

        {/* Grid */}
        <Text style={s.section}>{cat === 'All' ? 'All apps' : cat}</Text>
        <View style={s.grid}>
          {list.map((a) => {
            const on = installed.has(a.id);
            return (
              <View key={a.id} style={s.card}>
                <View style={s.cardTop}>
                  <View style={[s.iconBox, { backgroundColor: `${a.color}22` }]}>
                    <MaterialCommunityIcons name={a.icon as never} size={22} color={a.color} />
                  </View>
                  {a.core && <Text style={s.coreTag}>CORE</Text>}
                </View>
                <Text style={s.cardName}>{a.name}</Text>
                <Text style={s.cardBlurb} numberOfLines={2}>{a.blurb}</Text>
                <Pressable
                  onPress={() => !a.core && toggle(a.id)}
                  style={[s.addBtn, on ? s.addBtnDone : s.addBtnOpen, a.core && s.addBtnCore]}
                >
                  <MaterialCommunityIcons
                    name={on ? 'check' : 'plus'}
                    size={15}
                    color={on ? '#78f294' : '#071008'}
                  />
                  <Text style={[s.addText, on && s.addTextDone]}>
                    {a.core ? 'Installed' : on ? 'Added' : 'Add'}
                  </Text>
                </Pressable>
              </View>
            );
          })}
        </View>

        <Text style={s.note}>Concept marketplace — “Add to hat” previews the flow. Pushing apps to the device comes next.</Text>
      </ScrollView>
    </SafeAreaView>
  );
}

const s = StyleSheet.create({
  safe: { flex: 1, backgroundColor: '#090c12' },
  container: { padding: 18, paddingBottom: 44, gap: 12 },
  eyebrow: { color: '#8a94a8', fontSize: 11, fontWeight: '800', letterSpacing: 1.6, marginTop: 8 },
  titleRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
  title: { color: '#f4f7fb', fontSize: 26, fontWeight: '800' },
  countPill: { flexDirection: 'row', alignItems: 'center', gap: 6, backgroundColor: '#151b25', paddingHorizontal: 10, paddingVertical: 6, borderRadius: 999 },
  countText: { color: '#cbd3df', fontSize: 12, fontWeight: '700' },
  search: { flexDirection: 'row', alignItems: 'center', gap: 8, backgroundColor: '#05070a', borderColor: '#28303d', borderWidth: 1, borderRadius: 12, paddingHorizontal: 12 },
  searchInput: { flex: 1, color: '#f4f7fb', paddingVertical: 11, fontSize: 14 },
  chips: { gap: 8, paddingVertical: 2, paddingRight: 8 },
  chip: { backgroundColor: '#151b25', borderColor: '#2b3441', borderWidth: 1, borderRadius: 999, paddingHorizontal: 14, paddingVertical: 8 },
  chipOn: { backgroundColor: '#b8ff65', borderColor: '#b8ff65' },
  chipText: { color: '#cbd3df', fontWeight: '700', fontSize: 13 },
  chipTextOn: { color: '#071008' },
  featured: { backgroundColor: '#121720', borderColor: '#2b3441', borderWidth: 1, borderRadius: 20, padding: 16, gap: 8 },
  featuredTop: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
  featuredTag: { color: '#b8ff65', fontSize: 10, fontWeight: '900', letterSpacing: 1.4 },
  featIcon: { width: 52, height: 52, borderRadius: 14, alignItems: 'center', justifyContent: 'center' },
  featuredName: { color: '#f4f7fb', fontSize: 22, fontWeight: '800' },
  featuredBlurb: { color: '#96a0b5', fontSize: 13, lineHeight: 19 },
  getBtn: { flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 8, minHeight: 46, borderRadius: 13, marginTop: 4 },
  getBtnOn: { backgroundColor: '#b8ff65' },
  getBtnDone: { backgroundColor: '#1d2a20', borderColor: '#2f5138', borderWidth: 1 },
  getText: { color: '#071008', fontWeight: '900', fontSize: 15 },
  getTextDone: { color: '#78f294' },
  section: { color: '#f4f7fb', fontSize: 16, fontWeight: '800', marginTop: 6 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  card: { width: '48%', flexGrow: 1, backgroundColor: '#121720', borderColor: '#242c39', borderWidth: 1, borderRadius: 16, padding: 13, gap: 7 },
  cardTop: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
  iconBox: { width: 40, height: 40, borderRadius: 11, alignItems: 'center', justifyContent: 'center' },
  coreTag: { color: '#667085', fontSize: 9, fontWeight: '900', letterSpacing: 0.8, backgroundColor: '#0d1017', borderRadius: 5, paddingHorizontal: 6, paddingVertical: 3, overflow: 'hidden' },
  cardName: { color: '#e9edf4', fontWeight: '800', fontSize: 15 },
  cardBlurb: { color: '#727c90', fontSize: 12, lineHeight: 16, minHeight: 32 },
  addBtn: { flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 6, minHeight: 36, borderRadius: 10, marginTop: 2 },
  addBtnOpen: { backgroundColor: '#b8ff65' },
  addBtnDone: { backgroundColor: '#1d2a20', borderColor: '#2f5138', borderWidth: 1 },
  addBtnCore: { backgroundColor: '#151b25', borderColor: '#2b3441', borderWidth: 1 },
  addText: { color: '#071008', fontWeight: '900', fontSize: 13 },
  addTextDone: { color: '#78f294' },
  note: { color: '#667085', fontSize: 11, textAlign: 'center', lineHeight: 16, marginTop: 6 },
});
