export function encodeMessage(msg: unknown): Uint8Array {
  return new TextEncoder().encode(JSON.stringify(msg) + '\n');
}

export function chunkForMtu(bytes: Uint8Array, mtu: number): Uint8Array[] {
  if (mtu < 1) throw new Error('mtu must be >= 1');
  const chunks: Uint8Array[] = [];
  for (let i = 0; i < bytes.length; i += mtu) chunks.push(bytes.slice(i, i + mtu));
  return chunks;
}

export class MessageDecoder {
  private buf = '';
  push(chunk: Uint8Array): unknown[] {
    this.buf += new TextDecoder().decode(chunk);
    const out: unknown[] = [];
    let nl: number;
    while ((nl = this.buf.indexOf('\n')) !== -1) {
      const line = this.buf.slice(0, nl).trim();
      this.buf = this.buf.slice(nl + 1);
      if (line) out.push(JSON.parse(line));
    }
    return out;
  }
}
