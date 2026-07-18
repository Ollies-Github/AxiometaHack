export type WidgetId = 'prompt' | 'speaker' | 'mood' | 'timer' | 'caption' | 'status';

export interface Widget {
  id: WidgetId;
  name: string;
  icon: string;
  color: string;
  wearerLabel: string;
  outwardLabel: string;
}

export interface HatLayout {
  version: 1;
  activeWidget: WidgetId;
  brightness: number;
  volume: number;
}

export type PhoneToHatMessage =
  | { type: 'layout.set'; payload: HatLayout }
  | { type: 'widget.trigger'; payload: { id: WidgetId } }
  | { type: 'display.text'; payload: { target: 'wearer' | 'outward'; text: string } };

export type HatToPhoneMessage =
  | { type: 'hat.ready'; payload: { name: string; firmware: string } }
  | { type: 'encoder'; payload: { delta: -1 | 1; pressed: boolean } }
  | { type: 'telemetry'; payload: { batteryPercent?: number; rssi?: number } };

export * from './messages';
export * from './codec';
