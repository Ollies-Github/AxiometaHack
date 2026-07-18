export type AppInfo = { id: string; name: string };

// ---- Music app (Spotify now-playing / remote) ------------------------------
// Metadata only — audio plays on the phone. `music.state` is pushed to the hat;
// the hat sends `music.command` back for transport control.
export type MusicState = {
  title: string;
  artist: string;
  positionMs: number;
  durationMs: number;
  isPlaying: boolean;
  volume: number; // 0..100
};
export type MusicAction = 'playpause' | 'next' | 'prev' | 'volume';

// ---- Maps app (turn-by-turn) -----------------------------------------------
export type Turn =
  | 'left' | 'right' | 'straight' | 'slight-left' | 'slight-right' | 'uturn';
export type Maneuver = {
  turn: Turn;
  distanceM: number;
  street: string;
};

export type PhoneToHat =
  | { type: 'app.setActive'; payload: { id: string } }
  | { type: 'display.render'; payload: { lines: string[] } }
  | { type: 'music.state'; payload: MusicState }
  | { type: 'maps.maneuver'; payload: Maneuver }
  | { type: 'ping' };

export type HatToPhone =
  | { type: 'hat.ready'; payload: { name: string; firmware: string; apps: AppInfo[] } }
  | { type: 'encoder'; payload: { delta: number; pressed: boolean } }
  | { type: 'app.changed'; payload: { id: string } }
  | { type: 'music.command'; payload: { action: MusicAction; value?: number } }
  | { type: 'pong' };
