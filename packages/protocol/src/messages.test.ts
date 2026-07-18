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
});
