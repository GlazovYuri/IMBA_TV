// Кадры экрана для видео: собирает tools/brag/harness.cpp с кодом прошивки и GyverOLED,
// запускает и переводит записи в <out>/oled-data.js (window.OLED) и <out>/oled-events.json
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const HERE = path.dirname(new URL(import.meta.url).pathname);
const ROOT = path.resolve(HERE, '../..');
const SCENES = ['hook', 'link', 'ride', 'alarm', 'charging', 'update', 'poweroff', 'logo',
  'wave', 'break', 'explosion', 'cycle'];

function findGyverOled() {
  for (const p of [process.env.GYVEROLED, path.join(os.homedir(), 'Documents/Arduino/libraries/GyverOLED/src'), path.join(os.homedir(), 'Arduino/libraries/GyverOLED/src')])
    if (p && fs.existsSync(path.join(p, 'GyverOLED.h'))) return p;
  throw new Error('Не найдена библиотека GyverOLED, задайте GYVEROLED=путь/к/GyverOLED/src');
}

// копия скетча с номером последнего релиза в version.h, как при сборке в GitHub Actions
function sketchCopy(work) {
  const dir = path.join(work, 'IMBA_TV');
  fs.rmSync(dir, { recursive: true, force: true });
  fs.cpSync(path.join(ROOT, 'IMBA_TV'), dir, { recursive: true });
  let version = 'dev';
  try {
    version = execFileSync('git', ['describe', '--tags', '--abbrev=0', '--exclude', '*-dev*'], { cwd: ROOT }).toString().trim().replace(/^v/, '');
  } catch {}
  fs.writeFileSync(path.join(dir, 'version.h'), `#pragma once\n\n#define FW_VERSION "${version}"\n`);
  return dir;
}

// список исходников прошивки тот же, что у превью сайта
function firmwareSources(sketch) {
  const py = fs.readFileSync(path.join(ROOT, 'tools/render_previews.py'), 'utf8');
  const m = py.match(/^SOURCES = \[(.*)\]$/m);
  return [...m[1].matchAll(/'(\w+)'/g)].map(x => path.join(sketch, x[1] + '.cpp'));
}

export function buildOled(outDir) {
  const work = path.join(outDir, 'oled');
  fs.mkdirSync(work, { recursive: true });
  const exe = path.join(work, 'harness');
  const sketch = sketchCopy(work);
  execFileSync(process.env.CXX || 'c++', ['-std=c++17', '-O2', '-w',
    '-I', path.join(ROOT, 'tools/previews/shim'), '-I', findGyverOled(), '-I', sketch,
    path.join(HERE, 'harness.cpp'), ...firmwareSources(sketch), '-o', exe], { stdio: 'inherit' });
  execFileSync(exe, [work, '0x5EED1234'], { stdio: 'inherit' });

  const FB = 1024, REC = 4 + 2 + FB;
  const data = {}, events = {};
  for (const name of SCENES) {
    const buf = fs.readFileSync(path.join(work, name + '.bin'));
    const frames = [];
    for (let pos = 0; buf.length - pos >= REC; pos += REC)
      frames.push([buf.readUInt32LE(pos), buf[pos + 4], buf[pos + 5], buf.subarray(pos + 6, pos + REC).toString('base64')]);
    const end = buf.readUInt32LE(frames.length * REC);
    const t0 = frames[0][0];
    // подряд одинаковые кадры склеиваются, время от начала сцены
    const merged = [];
    for (const f of frames) {
      const last = merged[merged.length - 1];
      if (last && last[1] === f[1] && last[2] === f[2] && last[3] === f[3]) continue;
      merged.push([f[0] - t0, f[1], f[2], f[3]]);
    }
    data[name] = { end: end - t0, frames: merged };
    const inv = [];
    for (let i = 1; i < merged.length; i++) if (merged[i][2] && !merged[i - 1][2]) inv.push(merged[i][0]);
    events[name] = { invert_on: inv, end: end - t0, frames: merged.length };
  }
  fs.writeFileSync(path.join(outDir, 'oled-data.js'), 'window.OLED = ' + JSON.stringify(data) + ';\n');
  fs.writeFileSync(path.join(outDir, 'oled-events.json'), JSON.stringify(events, null, 1));
  return events;
}

if (process.argv[1] === new URL(import.meta.url).pathname) {
  const ev = buildOled(path.resolve(process.argv[2] || 'brag-output/work'));
  for (const [k, v] of Object.entries(ev)) console.log(k, v.frames, 'кадров', v.end, 'мс', v.invert_on.length ? 'мигания ' + v.invert_on.join(',') : '');
}
