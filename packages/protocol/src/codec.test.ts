import { describe, it, expect } from 'vitest';
import { encodeMessage, chunkForMtu, MessageDecoder } from './codec';

describe('codec', () => {
  it('encodes a message as newline-terminated JSON', () => {
    const bytes = encodeMessage({ type: 'ping' });
    expect(new TextDecoder().decode(bytes)).toBe('{"type":"ping"}\n');
  });

  it('chunks bytes to the mtu', () => {
    const bytes = new Uint8Array([1, 2, 3, 4, 5]);
    expect(chunkForMtu(bytes, 2).map((c) => Array.from(c))).toEqual([[1, 2], [3, 4], [5]]);
  });

  it('reassembles a message split across chunks', () => {
    const dec = new MessageDecoder();
    const full = encodeMessage({ type: 'app.setActive', payload: { id: 'clock' } });
    const chunks = chunkForMtu(full, 4);
    const out = chunks.flatMap((c) => dec.push(c));
    expect(out).toEqual([{ type: 'app.setActive', payload: { id: 'clock' } }]);
  });

  it('emits two messages from one buffer and ignores a trailing partial', () => {
    const dec = new MessageDecoder();
    const a = encodeMessage({ type: 'ping' });
    const b = encodeMessage({ type: 'pong' });
    const partial = new TextEncoder().encode('{"type":"pi');
    const buf = new Uint8Array([...a, ...b, ...partial]);
    expect(dec.push(buf)).toEqual([{ type: 'ping' }, { type: 'pong' }]);
  });

  it('reassembles a multi-byte UTF-8 character split across small chunks', () => {
    const dec = new MessageDecoder();
    const original = { type: 'note', payload: { text: 'café 🎉' } };
    const full = encodeMessage(original);
    const chunks = chunkForMtu(full, 3);
    const out = chunks.flatMap((c) => dec.push(c));
    expect(out).toEqual([original]);
  });

  it('skips a malformed line but keeps the valid messages around it', () => {
    const dec = new MessageDecoder();
    const a = encodeMessage({ type: 'a' });
    const b = encodeMessage({ type: 'b' });
    const garbage = new TextEncoder().encode('not json at all\n');
    const buf = new Uint8Array([...a, ...garbage, ...b]);
    expect(dec.push(buf)).toEqual([{ type: 'a' }, { type: 'b' }]);
  });
});
