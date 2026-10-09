// Блок настроек, как его кладёт в прошивку IMBA_TV/config.cpp, — из сгенерированного config_gen.h
import { readFileSync } from 'node:fs';

const header = readFileSync(new URL('../../IMBA_TV/config_gen.h', import.meta.url), 'utf8');

function cString(name) {
  const body = header.split(`#define ${name} \\\n`)[1].split('\n\n')[0];
  return [...body.matchAll(/"((?:[^"\\]|\\.)*)"/g)]
    .map((m) => m[1].replace(/\\(.)/g, '$1'))
    .join('');
}

export const schemaText = cString('FW_CONFIG_SCHEMA');
export const schema = JSON.parse(schemaText);
export const fields = schema.groups.flatMap((g) => g.fields);
export const defaults = header.match(/#define FW_CONFIG_DEFAULTS \{ ([^}]*) \}/)[1].split(', ').map(Number);

export function makeBlock(values = defaults) {
  const schemaBytes = new TextEncoder().encode(`${schemaText}\0`);
  const size = fields.reduce((n, f) => n + f.size, 0);
  const block = new Uint8Array(12 + size + schemaBytes.length);
  const v = new DataView(block.buffer);
  block.set(new TextEncoder().encode('IMBA_CFG'));
  v.setUint16(8, size, true);
  v.setUint16(10, schemaBytes.length, true);
  fields.forEach((f, i) => {
    if (f.size === 2) v.setUint16(12 + f.offset, values[i], true);
    else v.setUint8(12 + f.offset, values[i]);
  });
  block.set(schemaBytes, 12 + size);
  return block;
}

// Образ приложения с блоком настроек внутри
export function withBlock(bin, at = 1000, block = makeBlock()) {
  const out = new Uint8Array(Math.max(bin.length, at + block.length + 100)).fill(0xff);
  out.set(bin);
  out.set(block, at);
  return out;
}
