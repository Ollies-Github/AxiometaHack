export function encodeMessage(msg: unknown): Uint8Array {
  return new TextEncoder().encode(JSON.stringify(msg) + '\n');
}

export function chunkForMtu(bytes: Uint8Array, mtu: number): Uint8Array[] {
  if (mtu < 1) throw new Error('mtu must be >= 1');
  const chunks: Uint8Array[] = [];
  for (let i = 0; i < bytes.length; i += mtu) chunks.push(bytes.slice(i, i + mtu));
  return chunks;
}

const NEWLINE = 0x0a;

export class MessageDecoder {
  private pending: number[] = [];
  private readonly decoder = new TextDecoder();

  push(chunk: Uint8Array): unknown[] {
    // Buffer raw bytes rather than decoded text: BLE chunks split at byte
    // offsets, which can land in the middle of a multi-byte UTF-8 sequence.
    // Decoding each chunk independently would corrupt those characters, so
    // we only decode once we have a complete line's worth of bytes.
    for (let i = 0; i < chunk.length; i++) this.pending.push(chunk[i]);

    const out: unknown[] = [];
    let nl: number;
    while ((nl = this.pending.indexOf(NEWLINE)) !== -1) {
      const lineBytes = Uint8Array.from(this.pending.slice(0, nl));
      this.pending = this.pending.slice(nl + 1);
      const line = this.decoder.decode(lineBytes).trim();
      if (!line) continue;
      try {
        out.push(JSON.parse(line));
      } catch {
        // Skip a malformed line without discarding messages already
        // collected before or after it in this push.
      }
    }
    return out;
  }
}
