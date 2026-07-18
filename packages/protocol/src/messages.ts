export type AppInfo = { id: string; name: string };

export type PhoneToHat =
  | { type: 'app.setActive'; payload: { id: string } }
  | { type: 'display.render'; payload: { lines: string[] } }
  | { type: 'ping' };

export type HatToPhone =
  | { type: 'hat.ready'; payload: { name: string; firmware: string; apps: AppInfo[] } }
  | { type: 'encoder'; payload: { delta: number; pressed: boolean } }
  | { type: 'app.changed'; payload: { id: string } }
  | { type: 'pong' };
