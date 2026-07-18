import { describe, it, expect } from 'vitest';
import type { PhoneToHat, HatToPhone } from './messages';

describe('message types', () => {
  it('round-trips a display.render message through JSON', () => {
    const msg: PhoneToHat = { type: 'display.render', payload: { lines: ['hi', 'there'] } };
    const parsed = JSON.parse(JSON.stringify(msg)) as PhoneToHat;
    expect(parsed).toEqual(msg);
  });

  it('round-trips a hat.ready message with apps', () => {
    const msg: HatToPhone = {
      type: 'hat.ready',
      payload: { name: 'HatOS', firmware: 'foundation-0.1', apps: [{ id: 'launcher', name: 'Home' }] },
    };
    const parsed = JSON.parse(JSON.stringify(msg)) as HatToPhone;
    expect(parsed).toEqual(msg);
  });

  it('round-trips a music.state message', () => {
    const msg: PhoneToHat = {
      type: 'music.state',
      payload: { title: 'Placeholder Track', artist: 'Placeholder Artist', positionMs: 42000, durationMs: 180000, isPlaying: true, volume: 65 },
    };
    expect(JSON.parse(JSON.stringify(msg)) as PhoneToHat).toEqual(msg);
  });

  it('round-trips a music.command message', () => {
    const msg: HatToPhone = { type: 'music.command', payload: { action: 'volume', value: 70 } };
    expect(JSON.parse(JSON.stringify(msg)) as HatToPhone).toEqual(msg);
  });

  it('round-trips a maps.maneuver message', () => {
    const msg: PhoneToHat = {
      type: 'maps.maneuver',
      payload: { turn: 'slight-left', distanceM: 250, street: 'Example Road' },
    };
    expect(JSON.parse(JSON.stringify(msg)) as PhoneToHat).toEqual(msg);
  });
});
