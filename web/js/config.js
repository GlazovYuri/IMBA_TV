// Настройки, встроенные в прошивку (IMBA_TV/config.cpp): блок с magic «IMBA_CFG»,
// размерами, значениями и описанием формы (IMBA_TV/config.json с адресами полей).
// Сайт меняет значения прямо в образе — пересборка прошивки не нужна.

import { FirmwareError } from './firmware.js';

const MAGIC = new TextEncoder().encode('IMBA_CFG');
const HEADER_SIZE = MAGIC.length + 4;
const TYPES = { bool: 1, u8: 1, u16: 2 };

function indexOf(bytes, pattern) {
  outer: for (let i = bytes.indexOf(pattern[0]); i >= 0 && i <= bytes.length - pattern.length;
    i = bytes.indexOf(pattern[0], i + 1)) {
    for (let j = 1; j < pattern.length; j++) if (bytes[i + j] !== pattern[j]) continue outer;
    return i;
  }
  return -1;
}

// { offset, size, schema, fields } или null, если прошивка не поддерживает настройку
export function findConfig(bin) {
  const at = indexOf(bin, MAGIC);
  if (at < 0) return null;

  const v = new DataView(bin.buffer, bin.byteOffset, bin.byteLength);
  const broken = new FirmwareError('Блок настроек в прошивке повреждён');
  if (at + HEADER_SIZE > bin.length) throw broken;
  const size = v.getUint16(at + 8, true);
  const schemaSize = v.getUint16(at + 10, true);
  const offset = at + HEADER_SIZE;
  if (offset + size + schemaSize > bin.length) throw broken;

  let schema;
  try {
    const raw = bin.subarray(offset + size, offset + size + schemaSize);
    const end = raw.indexOf(0);
    schema = JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(end < 0 ? raw : raw.subarray(0, end)));
  } catch {
    throw broken;
  }

  const fields = (schema.groups ?? []).flatMap((g) => g.fields ?? []);
  for (const f of fields) {
    if (TYPES[f.type] !== f.size || f.offset < 0 || f.offset + f.size > size) throw broken;
  }
  return { offset, size, schema, fields };
}

function readField(v, cfg, f) {
  const at = cfg.offset + f.offset;
  if (f.type === 'bool') return v.getUint8(at) !== 0;
  return f.size === 2 ? v.getUint16(at, true) : v.getUint8(at);
}

export function readValues(bin, cfg) {
  const v = new DataView(bin.buffer, bin.byteOffset, bin.byteLength);
  return Object.fromEntries(cfg.fields.map((f) => [f.key, readField(v, cfg, f)]));
}

export function defaultValues(cfg) {
  return Object.fromEntries(cfg.fields.map((f) => [f.key, f.default]));
}

// Проверяет значение по описанию поля; возвращает нормализованное значение или бросает FirmwareError
export function checkValue(f, value) {
  if (f.type === 'bool') {
    if (typeof value !== 'boolean') throw new FirmwareError(`«${f.label}»: нужно да или нет`);
    return value;
  }
  const n = typeof value === 'string' && value.trim() !== '' ? Number(value) : value;
  if (!Number.isInteger(n)) throw new FirmwareError(`«${f.label}»: нужно целое число`);
  if (f.options) {
    if (!f.options.some((o) => o.value === n)) throw new FirmwareError(`«${f.label}»: недопустимое значение`);
    return n;
  }
  const min = f.min ?? 0;
  const max = f.max ?? (f.size === 2 ? 0xffff : 0xff);
  if (n < min || n > max) {
    throw new FirmwareError(`«${f.label}»: значение должно быть от ${min} до ${max}${f.unit ? ` ${f.unit}` : ''}`);
  }
  return n;
}

// Поле показывается, только если выполнены все условия depends
export function isVisible(f, values) {
  return Object.entries(f.depends ?? {}).every(([key, expected]) => values[key] === expected);
}

// Новый образ с записанными настройками
export function applyValues(bin, cfg, values) {
  const out = bin.slice();
  const v = new DataView(out.buffer);
  for (const f of cfg.fields) {
    const value = checkValue(f, values[f.key]);
    const at = cfg.offset + f.offset;
    if (f.type === 'bool') v.setUint8(at, value ? 1 : 0);
    else if (f.size === 2) v.setUint16(at, value, true);
    else v.setUint8(at, value);
  }
  return out;
}

// Значения, сохранённые раньше, поверх значений из прошивки — если они подходят этой версии
export function mergeValues(cfg, base, saved) {
  const values = { ...base };
  for (const f of cfg.fields) {
    if (!saved || !(f.key in saved)) continue;
    try {
      values[f.key] = checkValue(f, saved[f.key]);
    } catch {
      // настройка изменилась в новой версии прошивки — оставляем значение из прошивки
    }
  }
  return values;
}
