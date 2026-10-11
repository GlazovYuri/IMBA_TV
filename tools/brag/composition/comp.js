// Видео IMBA TV: каждый кадр - чистая функция времени. window.renderFrame(t) рисует момент t (секунды).
// Экран везде рисуется из кадров настоящей прошивки (window.OLED, tools/brag/harness.cpp).
import { createDevice, SCREEN_Y, WIN_Z } from './device.js';

// ---------------------------------------------------------------- время: 125 BPM, доля 0,48 с
const BEAT = 0.48;
const at = n => n * BEAT;
const S = {
  hook: 0, land: at(2.5), on: at(3), boom: at(4), cut: at(6),
  link: at(14), open: at(22), screen: at(30), alarm: at(36),
  editor: at(42), anims: at(54), flash: at(62), outro: at(74), end: at(84),
};
const EDIT = { gear: S.editor + 1.06, zone: S.editor + 1.86, temp: S.editor + 2.41, done: S.editor + 2.96 };
const FLASH = { tab: S.flash + 1.5, click: S.flash + 2.3 };
const CRT = S.outro + 0.96;
const DURATION = Math.round(S.end * 30) / 30;

const clamp = (x, a = 0, b = 1) => Math.min(b, Math.max(a, x));
const lerp = (a, b, k) => a + (b - a) * k;
const prog = (t, a, b) => clamp((t - a) / (b - a));
const E = {
  out3: k => 1 - Math.pow(1 - k, 3),
  out5: k => 1 - Math.pow(1 - k, 5),
  in3: k => k * k * k,
  io3: k => (k < 0.5 ? 4 * k * k * k : 1 - Math.pow(-2 * k + 2, 3) / 2),
  io4: k => (k < 0.5 ? 8 * k ** 4 : 1 - Math.pow(-2 * k + 2, 4) / 2),
};
const $ = id => document.getElementById(id);
const rectLerp = (a, b, k) => ({ x: lerp(a.x, b.x, k), y: lerp(a.y, b.y, k), w: lerp(a.w, b.w, k), h: lerp(a.h, b.h, k) });
const lerp3 = (a, b, k) => a.map((v, i) => lerp(v, b[i], k));

function show(el, o, dx = 0, dy = 0) {
  el.style.opacity = o;
  el.style.transform = `translate(${dx}px, ${dy}px)`;
  el.style.visibility = o <= 0.001 ? 'hidden' : 'visible';
}
// появление [a, a+d] и уход [b, b+d2]
function inOut(t, a, d, b, d2 = 0.22) {
  const kin = E.out3(prog(t, a, a + d));
  const kout = b == null ? 0 : E.in3(prog(t, b, b + d2));
  return { kin, kout, o: kin * (1 - kout) };
}

// ---------------------------------------------------------------- кадры экрана
const bitsCache = new Map();
function bitsOf(b64) {
  let a = bitsCache.get(b64);
  if (a) return a;
  const bin = atob(b64);
  a = new Uint8Array(8192);
  for (let i = 0; i < 8192; i++) a[i] = (bin.charCodeAt(i >> 3) >> (7 - (i & 7))) & 1;
  bitsCache.set(b64, a);
  return a;
}
const brCache = new Map();
function frameBrightness(f) {
  const key = f[3] + f[1] + ':' + f[2];
  let out = brCache.get(key);
  if (out) return out;
  const bits = bitsOf(f[3]);
  const k = 0.15 + 0.85 * f[1] / 255;
  out = new Float32Array(8192);
  for (let i = 0; i < 8192; i++) out[i] = f[2] ? (bits[i] ? 0 : k) : (bits[i] ? k : 0);
  brCache.set(key, out);
  return out;
}
function frameIndex(frames, ms) {
  if (ms < frames[0][0]) return 0;
  let lo = 0, hi = frames.length - 1;
  while (lo < hi) {
    const mid = (lo + hi + 1) >> 1;
    if (frames[mid][0] <= ms) lo = mid; else hi = mid - 1;
  }
  return lo;
}
// яркость точек в момент ms, усреднённая по выдержке кадра видео (1/30 с)
function sampleOled(name, ms, shutter = 1000 / 30) {
  const fr = OLED[name].frames;
  const a = ms - shutter / 2, b = ms + shutter / 2;
  const parts = [];
  for (let i = frameIndex(fr, a); i < fr.length; i++) {
    if (fr[i][0] > b && i > 0) break;
    const s = Math.max(a, i === 0 ? -1e9 : fr[i][0]);
    const e = Math.min(b, i + 1 < fr.length ? fr[i + 1][0] : 1e12);
    if (e > s) parts.push([i, e - s]);
  }
  if (parts.length === 1) return frameBrightness(fr[parts[0][0]]);
  const out = new Float32Array(8192);
  let total = 0;
  for (const [i, w] of parts) {
    const br = frameBrightness(fr[i]);
    for (let j = 0; j < 8192; j++) out[j] += br[j] * w;
    total += w;
  }
  for (let j = 0; j < 8192; j++) out[j] /= total;
  return out;
}

const ON = [238, 240, 242], PANEL = [14, 14, 14], OFFPX = [20, 20, 20];
const LUT = [];
for (let q = 0; q <= 64; q++) LUT.push(`rgb(${ON.map((on, i) => Math.round(OFFPX[i] + (on - OFFPX[i]) * q / 64))})`);

// точки экрана в прямоугольник rect, без свечения: так выглядит настоящий OLED
function paintOled(ctx, rect, br) {
  const p = rect.w / 128;
  const gap = p >= 3 ? Math.max(1, Math.round(p * 0.13)) : 0;
  const xs = new Int32Array(129), ys = new Int32Array(65);
  for (let i = 0; i <= 128; i++) xs[i] = Math.round(rect.x + i * p);
  for (let i = 0; i <= 64; i++) ys[i] = Math.round(rect.y + i * p);
  let last = -1;
  for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
    const v = br ? br[y * 128 + x] : 0;
    const q = Math.max(0, Math.min(64, Math.round(v * 64)));
    if (q !== last) { ctx.fillStyle = LUT[q]; last = q; }
    ctx.fillRect(xs[x], ys[y], xs[x + 1] - xs[x] - gap, ys[y + 1] - ys[y] - gap);
  }
}
// экран на холсте в координатах кадра: rect - 128×64, pad - чёрное поле вокруг
function drawOled(cv, rect, pad, br) {
  const x0 = Math.round(rect.x - pad), y0 = Math.round(rect.y - pad);
  const W = Math.round(rect.w + 2 * pad), H = Math.round(rect.h + 2 * pad);
  if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; }
  Object.assign(cv.style, { left: x0 + 'px', top: y0 + 'px', width: W + 'px', height: H + 'px' });
  const ctx = cv.getContext('2d');
  ctx.fillStyle = `rgb(${PANEL})`;
  ctx.fillRect(0, 0, W, H);
  paintOled(ctx, { x: rect.x - x0, y: rect.y - y0, w: rect.w, h: rect.h }, br);
}
const padOf = r => r.w / 128 * 4;
function frameBox(el, r, pad) {
  Object.assign(el.style, { left: (r.x - pad - 3) + 'px', top: (r.y - pad - 3) + 'px', width: (r.w + 2 * pad + 6) + 'px', height: (r.h + 2 * pad + 6) + 'px' });
}

// ---------------------------------------------------------------- вступление в 3D
const tex = document.createElement('canvas');
tex.width = 2112; tex.height = 1408;            // пластина экрана 33×22 мм, 64 px/мм
const ACT = { x: (2112 - 29.4 * 64) / 2, y: (1408 - 14.7 * 64) / 2, w: 29.4 * 64, h: 14.7 * 64 };
let dev;
function paintScreen(br) {
  const g = tex.getContext('2d');
  g.fillStyle = `rgb(${PANEL})`;
  g.fillRect(0, 0, tex.width, tex.height);
  if (br) paintOled(g, ACT, br);
}
const rad = d => d * Math.PI / 180;
function sph(az, el, dist, target) {
  const a = rad(az), e = rad(el);
  return { pos: [target[0] + dist * Math.cos(e) * Math.sin(a), target[1] + dist * Math.sin(e), target[2] + dist * Math.cos(e) * Math.cos(a)], target };
}
const pose = (az, el, dist, target) => ({ az, el, dist, target });
const poseLerp = (A, B, k) => ({ az: lerp(A.az, B.az, k), el: lerp(A.el, B.el, k), dist: Math.exp(lerp(Math.log(A.dist), Math.log(B.dist), k)), target: lerp3(A.target, B.target, k) });
// камера над экраном: видимая область OLED 29,4 мм ровно на ширину кадра, как экран на листе после склейки
const TOP = pose(0, 90, 33.2, [0, SCREEN_Y, WIN_Z]);
const HOOK0 = pose(-38, 30, 122, [0, 6.5, -1]);
const HOOK1 = pose(-28, 33, 112, [0, 6.5, -1]);

function state3d(t) {
  // корпус влетает с поворотом и садится на коврик
  const k = E.out3(prog(t, 0, S.land));
  const settle = Math.sin(Math.PI * prog(t, S.land - 0.12, S.land + 0.3));
  const device = {
    x: 70 * (1 - k), y: 46 * (1 - k), z: -40 * (1 - k),
    ry: lerp(-2.7, 0, E.io3(prog(t, 0, S.land + 0.1))),
    rx: 0.4 * (1 - k) - 0.03 * settle, rz: -0.25 * (1 - k),
  };
  // камера ведёт корпус, после включения уходит в экран
  const ks = E.io4(prog(t, S.on + 0.05, S.cut));
  let p = poseLerp(HOOK0, HOOK1, E.io3(prog(t, 0, S.on)));
  p.target = [p.target[0] + device.x * 0.5, p.target[1] + device.y * 0.6, p.target[2] + device.z * 0.5];
  p = poseLerp(p, TOP, ks);
  const up = ks > 0.999 ? [0, 0, -1] : [0, 1, 0];
  const press = prog(t, S.land + 0.04, S.land + 0.1) * (1 - prog(t, S.on, S.on + 0.06));
  const screen = t >= S.on ? sampleOled('hook', (t - S.on) * 1000 + 520) : null;
  return { cam: { ...sph(p.az, p.el, p.dist, p.target), fov: 28, up }, device, press, screen };
}

// ---------------------------------------------------------------- лист: экран и подписи
const RT = { x: 1032, y: 348, w: 768, h: 384 };   // титул
const RL = { x: 1296, y: 432, w: 512, h: 256 };   // схема подключения
const R3 = { x: 512, y: 418, w: 896, h: 448 };    // выноски
const R4 = { x: 152, y: 200, w: 896, h: 448 };    // тревога ШИМ
const RO = { x: 152, y: 348, w: 768, h: 384 };    // концовка
const FULL = { x: 0, y: 60, w: 1920, h: 960 };

function figState(t) {
  if (t < S.link) return { r: RT, o: 1 };
  if (t < S.open) return { r: rectLerp(RT, RL, E.io3(prog(t, S.link + 0.05, S.link + 0.65))), o: 1 };
  if (t < S.screen) return { r: RL, o: 1 - E.in3(prog(t, S.open, S.open + 0.3)) };
  if (t < S.outro) {
    const kin = E.out3(prog(t, S.screen, S.screen + 0.45));
    const r = rectLerp(R3, R4, E.io3(prog(t, S.alarm + 0.05, S.alarm + 0.6)));
    const sc = lerp(0.94, 1, kin);
    return { r: { x: r.x + r.w * (1 - sc) / 2, y: r.y + r.h * (1 - sc) / 2, w: r.w * sc, h: r.h * sc }, o: kin };
  }
  return { r: RO, o: 1 };
}
function figSource(t) {
  if (t < S.link) return ['hook', (t - S.on) * 1000 + 520];
  if (t < S.screen) return ['link', (t - (S.link + 0.85)) * 1000];
  if (t < S.alarm) return ['ride', (t - S.screen) * 1000];
  if (t < S.outro) return ['alarm', (t - S.alarm) * 1000];
  return ['poweroff', (t - (CRT - 1.5)) * 1000];
}
// камера листа: склейка с 3D, экран во весь кадр отъезжает на место рис. 1
function pageCam(t) {
  const s0 = FULL.w / RT.w, c = { x: RT.x + RT.w / 2, y: RT.y + RT.h / 2 };
  const k = E.io4(prog(t, S.cut, S.cut + 0.7));
  const s = Math.exp(lerp(Math.log(s0), 0, k));
  const sx = lerp(960, c.x, k), sy = lerp(540, c.y, k);
  return { s, cx: c.x - (sx - 960) / s, cy: c.y - (sy - 540) / s };
}
const toScreen = (cam, x, y) => ({ x: (x - cam.cx) * cam.s + 960, y: (y - cam.cy) * cam.s + 540 });

const NS = 'http://www.w3.org/2000/svg';
function svgAdd(svg, tag, attrs, text) {
  const e = document.createElementNS(NS, tag);
  for (const k in attrs) e.setAttribute(k, attrs[k]);
  if (text != null) e.textContent = text;
  svg.appendChild(e);
  return e;
}

// выноски к основному экрану
const leaders = [];
const LABELS = [
  { id: 'lb1', ax: 6.5, ay: 1, side: 'top' },
  { id: 'lb2', ax: 74, ay: 1, side: 'top' },
  { id: 'lb3', ax: 11, ay: 34, side: 'left' },
  { id: 'lb4', ax: 117, ay: 34, side: 'right' },
];
function initLeaders() {
  const p = R3.w / 128, pad = padOf(R3), svg = $('leaders');
  const fl = R3.x - pad - 3, fr = R3.x + R3.w + pad + 3, ft = R3.y - pad - 3;
  for (const L of LABELS) {
    const ax = R3.x + L.ax * p, ay = R3.y + L.ay * p, lb = $(L.id);
    let ink, yel, lx, ly;
    if (L.side === 'top') {
      const bottom = ft - 74;
      lx = ax - 21; ly = bottom - 44;
      ink = [ax, bottom + 10, ax, ft]; yel = [ax, ft + 3, ax, ay];
    } else if (L.side === 'left') {
      const right = fl - 70;
      lx = right - lb.offsetWidth; ly = ay - 22;
      ink = [right + 12, ay, fl, ay]; yel = [fl + 3, ay, ax, ay];
    } else {
      const left = fr + 70;
      lx = left; ly = ay - 22;
      ink = [left - 12, ay, fr, ay]; yel = [fr - 3, ay, ax, ay];
    }
    Object.assign(lb.style, { left: lx + 'px', top: ly + 'px' });
    const li = svgAdd(svg, 'line', { x1: ink[0], y1: ink[1], x2: ink[2], y2: ink[3], stroke: '#1d1d1b', 'stroke-width': 2.5 });
    const ly2 = svgAdd(svg, 'line', { x1: yel[0], y1: yel[1], x2: yel[2], y2: yel[3], stroke: '#f2c230', 'stroke-width': 3 });
    const dot = svgAdd(svg, 'rect', { x: yel[2] - 6, y: yel[3] - 6, width: 12, height: 12, fill: '#f2c230' });
    const len = a => Math.hypot(a[2] - a[0], a[3] - a[1]);
    leaders.push({ lb, li, ly: ly2, dot, l1: len(ink), l2: len(yel), side: L.side });
  }
}
function renderLeaders(t) {
  leaders.forEach((g, i) => {
    const a = S.screen + 0.55 + i * 0.2;
    const out = E.in3(prog(t, S.alarm, S.alarm + 0.22));
    const kl = E.out3(prog(t, a, a + 0.32));
    const dir = g.side === 'top' ? [0, 18] : g.side === 'left' ? [18, 0] : [-18, 0];
    show(g.lb, kl * (1 - out), dir[0] * (1 - kl), dir[1] * (1 - kl));
    const k1 = E.out3(prog(t, a + 0.08, a + 0.3)) * (1 - out);
    const k2 = E.out3(prog(t, a + 0.24, a + 0.5)) * (1 - out);
    g.li.setAttribute('stroke-dasharray', `${g.l1 * k1} ${g.l1 + 10}`);
    g.ly.setAttribute('stroke-dasharray', `${g.l2 * k2} ${g.l2 + 10}`);
    g.dot.setAttribute('opacity', E.out3(prog(t, a + 0.42, a + 0.6)) * (1 - out));
    g.li.setAttribute('opacity', k1 > 0 ? 1 : 0);
    g.ly.setAttribute('opacity', k2 > 0 ? 1 : 0);
  });
}

// график ШИМ и импульсы мигания (моменты инверсии экрана из стенда)
const CH = { x0: 170, x1: 1640, pwmTop: 50, pwmBot: 118, scrTop: 136, scrBot: 172, dur: S.editor - S.alarm };
const smooth = k => { k = clamp(k); return k * k * (3 - 2 * k); };
const pwmAt = tau => 62 + 35 * smooth(tau / 2.0);
const chartX = tau => CH.x0 + (CH.x1 - CH.x0) * tau / CH.dur;
const chartY = pwm => CH.pwmBot - (pwm - 55) / 45 * (CH.pwmBot - CH.pwmTop);
const chart = {};
function initChart() {
  const svg = $('chart');
  svgAdd(svg, 'rect', { x: 0.75, y: 0.75, width: 1678.5, height: 190.5, fill: 'none', stroke: '#1d1d1b', 'stroke-width': 1.5 });
  const lab = { fill: '#55544e', 'font-family': 'Sofia Sans Condensed', 'font-weight': 700, 'font-size': 20, 'letter-spacing': '0.08em' };
  svgAdd(svg, 'text', { ...lab, x: 26, y: (CH.pwmTop + CH.pwmBot) / 2 + 7 }, 'ШИМ');
  svgAdd(svg, 'text', { ...lab, x: 26, y: (CH.scrTop + CH.scrBot) / 2 + 7 }, 'ЭКРАН');
  const ty = chartY(80);
  svgAdd(svg, 'line', { x1: CH.x0, y1: ty, x2: CH.x1, y2: ty, stroke: '#1d1d1b', 'stroke-width': 1.5, 'stroke-dasharray': '7 6' });
  svgAdd(svg, 'text', { x: CH.x1 - 2, y: ty - 9, 'text-anchor': 'end', fill: '#1d1d1b', 'font-family': 'Sofia Sans Condensed', 'font-weight': 700, 'font-size': 21 }, 'порог 80%');
  svgAdd(svg, 'line', { x1: CH.x0, y1: CH.scrBot, x2: CH.x1, y2: CH.scrBot, stroke: '#c8c5b9', 'stroke-width': 1.5 });
  chart.below = svgAdd(svg, 'polyline', { fill: 'none', stroke: '#1d1d1b', 'stroke-width': 4, 'stroke-linejoin': 'round' });
  chart.above = svgAdd(svg, 'polyline', { fill: 'none', stroke: '#c23b18', 'stroke-width': 4.5, 'stroke-linejoin': 'round' });
  const fr = OLED.alarm.frames;
  chart.pulses = [];
  for (let i = 1; i < fr.length; i++) {
    if (!fr[i][2] || fr[i - 1][2]) continue;
    let j = i + 1;
    while (j < fr.length && fr[j][2]) j++;
    const t0 = fr[i][0] / 1000, t1 = (j < fr.length ? fr[j][0] : fr[i][0] + 50) / 1000;
    if (t0 > CH.dur) continue;
    chart.pulses.push({ t0, el: svgAdd(svg, 'rect', { x: chartX(t0), y: CH.scrTop, width: Math.max(10, chartX(t1) - chartX(t0)), height: CH.scrBot - CH.scrTop, fill: '#1d1d1b' }) });
  }
  chart.head = svgAdd(svg, 'line', { x1: 0, y1: 14, x2: 0, y2: 180, stroke: '#1d1d1b', 'stroke-width': 1.5 });
  chart.val = svgAdd(svg, 'text', { x: 0, y: 0, fill: '#1d1d1b', 'font-family': 'Sofia Sans Condensed', 'font-weight': 800, 'font-size': 30 }, '');
}
function renderChart(t) {
  const tau = clamp(t - S.alarm, 0, CH.dur);
  const below = [], above = [];
  for (let i = 0; i <= 120; i++) {
    const s = tau * i / 120, v = pwmAt(s);
    const pt = `${chartX(s).toFixed(1)},${chartY(v).toFixed(1)}`;
    if (v <= 80.5) below.push(pt);
    if (v >= 79.5) above.push(pt);
  }
  chart.below.setAttribute('points', below.join(' '));
  chart.above.setAttribute('points', above.join(' '));
  for (const p of chart.pulses) p.el.setAttribute('opacity', tau >= p.t0 ? 1 : 0);
  const hx = chartX(tau);
  chart.head.setAttribute('x1', hx); chart.head.setAttribute('x2', hx);
  const v = Math.round(pwmAt(tau));
  chart.val.textContent = v + '%';
  chart.val.setAttribute('x', Math.min(hx + 12, CH.x1 - 140));
  chart.val.setAttribute('y', chartY(v) - 12);
  chart.val.setAttribute('fill', v >= 80 ? '#c23b18' : '#1d1d1b');
}

// схема: колесо - LoEUC на телефоне - IMBA TV, пакеты бегут по Bluetooth
const link = {};
function initLink() {
  const svg = $('link-svg'), ink = '#1d1d1b';
  const g = svgAdd(svg, 'g', { transform: 'translate(300 560)', fill: 'none', stroke: ink, 'stroke-width': 8, 'stroke-linejoin': 'round', 'stroke-linecap': 'round' });
  svgAdd(g, 'circle', { cx: 0, cy: 30, r: 112 });
  svgAdd(g, 'circle', { cx: 0, cy: 30, r: 80, 'stroke-width': 4 });
  svgAdd(g, 'circle', { cx: 0, cy: 30, r: 14, fill: ink });
  svgAdd(g, 'path', { d: 'M -128 46 L -128 10 A 128 128 0 0 1 128 10 L 128 46', fill: '#e7e5dd' });
  svgAdd(g, 'path', { d: 'M -46 -116 L -46 -150 L 46 -150 L 46 -116', 'stroke-width': 9 });
  svgAdd(g, 'line', { x1: -150, y1: 58, x2: -92, y2: 58, 'stroke-width': 12 });
  const ph = svgAdd(svg, 'g', { transform: 'translate(840 560)', fill: 'none', stroke: ink, 'stroke-width': 8 });
  svgAdd(ph, 'rect', { x: -98, y: -190, width: 196, height: 380, rx: 30 });
  svgAdd(ph, 'rect', { x: -78, y: -150, width: 156, height: 290, rx: 6, 'stroke-width': 3, fill: '#e7e5dd' });
  svgAdd(ph, 'text', { x: 0, y: -80, 'text-anchor': 'middle', fill: ink, stroke: 'none', 'font-family': 'Sofia Sans Condensed', 'font-weight': 800, 'font-size': 40 }, 'LoEUC');
  for (let i = 0; i < 3; i++) svgAdd(ph, 'rect', { x: -56, y: -40 + i * 50, width: [112, 80, 96][i], height: 22, fill: '#8b8980', stroke: 'none' });
  link.lines = [];
  const devLeft = RL.x - padOf(RL) - 3 - 22;
  for (const [x0, x1] of [[460, 712], [968, devLeft]]) {
    const y = 560;
    svgAdd(svg, 'line', { x1: x0, y1: y, x2: x1, y2: y, stroke: '#8b8980', 'stroke-width': 3, 'stroke-dasharray': '10 10' });
    svgAdd(svg, 'text', { x: (x0 + x1) / 2, y: y - 26, 'text-anchor': 'middle', fill: '#55544e', 'font-family': 'Sofia Sans Condensed', 'font-weight': 700, 'font-size': 24, 'letter-spacing': '0.08em' }, 'BLUETOOTH');
    const dots = [0, 1, 2].map(() => svgAdd(svg, 'rect', { width: 16, height: 16, fill: '#f2c230', stroke: ink, 'stroke-width': 2.5 }));
    svgAdd(svg, 'path', { d: `M ${x1 - 18} ${y - 14} L ${x1} ${y} L ${x1 - 18} ${y + 14}`, fill: 'none', stroke: ink, 'stroke-width': 4 });
    link.lines.push({ x0, x1, y, dots });
  }
  Object.assign($('node-wheel').style, { left: '120px', top: '720px' });
  Object.assign($('node-phone').style, { left: '660px', top: '772px' });
  const nd = document.createElement('p');
  nd.className = 'v-node'; nd.textContent = 'IMBA TV';
  Object.assign(nd.style, { left: (RL.x + RL.w / 2 - 180) + 'px', top: (RL.y + RL.h + padOf(RL) + 40) + 'px' });
  $('link').appendChild(nd);
}
function renderLink(t) {
  link.lines.forEach((L, li) => {
    const start = S.link + 0.75 + li * 0.85;
    L.dots.forEach((d, i) => {
      const ph = (t - start - i * 0.22) / 0.85;
      const x = L.x0 + (L.x1 - L.x0 - 24) * (((ph % 1) + 1) % 1);
      d.setAttribute('x', x - 8); d.setAttribute('y', L.y - 8);
      d.setAttribute('opacity', ph >= 0 ? 1 : 0);
    });
  });
}

// ---------------------------------------------------------------- сайт: редактор экранов и прошивка
let site = {}, siteInfo = {};
const BROWSER = { x: 60, y: 96, w: 1180, h: 910 };
function browserY(t) {
  if (t >= S.editor && t < S.anims) return lerp(1100, BROWSER.y, E.out5(prog(t, S.editor, S.editor + 0.5)));
  if (t >= S.flash && t < S.outro + 0.5) return lerp(lerp(1100, BROWSER.y, E.out5(prog(t, S.flash, S.flash + 0.5))), 1100, E.in3(prog(t, S.outro, S.outro + 0.42)));
  return 1100;
}
const editorBase = (BROWSER.w - 6) / 1100;
function editorCam(t) {
  const Hv = (BROWSER.h - 6 - 62) / editorBase;
  const K0 = { z: 1, cx: 550, cy: Hv / 2 };
  const K1 = { z: 1.5, cx: 500, cy: 410 };
  const K2 = { z: 1.08, cx: 520, cy: Hv / 2 / 1.08 + 10 };
  const mix = (A, B, k) => ({ z: Math.exp(lerp(Math.log(A.z), Math.log(B.z), k)), cx: lerp(A.cx, B.cx, k), cy: lerp(A.cy, B.cy, k) });
  if (t < EDIT.gear + 0.15) return K0;
  if (t < EDIT.done + 0.15) return mix(K0, K1, E.io3(prog(t, EDIT.gear + 0.15, EDIT.gear + 0.55)));
  return mix(K1, K2, E.io3(prog(t, EDIT.done + 0.15, EDIT.done + 0.6)));
}
const editorState = t => t < EDIT.gear + 0.1 ? 'list' : t < EDIT.zone + 0.1 ? 'dialog' : t < EDIT.temp + 0.1 ? 'zone' : t < EDIT.done + 0.1 ? 'temp' : 'done';

const C = FLASH.click;
const WORK = [
  { label: 'Подключение к дисплею', a: C + 0.06, b: C + 0.3 },
  { label: 'Перезагрузка в загрузчик', a: C + 0.3, b: C + 0.54 },
  { label: 'Передача по Bluetooth', a: C + 0.54, b: C + 1.75 },
  { label: 'Загрузчик применяет прошивку', a: C + 1.75, b: C + 2.0 },
];
const STATUS = [
  [C + 0.06, 'wait', 'Подключаюсь к дисплею…'],
  [C + 0.3, 'wait', 'Перезагружаю дисплей в режим загрузчика…'],
  [C + 0.54, 'wait', 'Передаю прошивку по Bluetooth… Держите телефон рядом с дисплеем и не закрывайте страницу.'],
  [C + 1.75, 'wait', 'Загрузчик применяет прошивку…'],
  [C + 2.0, 'ok', 'Прошивка записана, дисплей перезагрузился. Если экран не включился, нажмите и удерживайте кнопку.'],
];
const fmt = n => n.toLocaleString('ru-RU');

async function initSite() {
  $('sprite').innerHTML = await (await fetch('/work/site/sprite.svg')).text();
  siteInfo = await (await fetch('/work/site/site.json')).json();
  $('rh-rev').textContent = siteInfo.version;
  const op3 = (await (await fetch('/work/site/op3-ble.html')).text()).replace(/ id="/g, ' id="s-');
  document.querySelector('#site main').innerHTML = op3;
  $('s-work-target').textContent = `${siteInfo.firmware.tag} → IMBA TV`;
  $('s-work-list').replaceChildren(...WORK.map(w => {
    const li = document.createElement('li');
    li.dataset.state = 'todo';
    li.innerHTML = `<span class="w-box"></span><span class="w-label">${w.label}</span><span class="w-time"></span>`;
    w.li = li;
    return li;
  }));
  await Promise.all([...document.querySelectorAll('#editor img')].map(i => i.decode()));
}
function setTab(ble) {
  $('s-tab-usb').setAttribute('aria-selected', String(!ble));
  $('s-tab-ble').setAttribute('aria-selected', String(ble));
  $('s-panel-usb').hidden = ble;
  $('s-panel-ble').hidden = !ble;
}
function measureSite() {
  const root = $('site');
  root.style.transform = 'none';
  const base = root.getBoundingClientRect();
  const r = el => { const b = el.getBoundingClientRect(); return { x: b.left - base.left, y: b.top - base.top, w: b.width, h: b.height }; };
  setTab(false);
  site.op3 = r($('s-op3'));
  site.tab = r($('s-tab-ble'));
  setTab(true);
  site.btn = r($('s-flash-ble'));
}
function siteCam(t, Wv, Hv) {
  const z2 = 1.2, z4 = 1.48;
  const K2 = { z: z2, cx: 96 + Wv / 2 / z2, cy: site.op3.y + 36 + Hv / 2 / z2 };
  const K3 = { z: z2, cx: K2.cx, cy: site.btn.y - 0.5 * Hv / z2 + Hv / 2 / z2 };
  const K4 = { z: z4, cx: site.btn.x - 36 + Wv / 2 / z4, cy: site.btn.y - 30 + Hv / 2 / z4 };
  const mix = (A, B, k) => ({ z: Math.exp(lerp(Math.log(A.z), Math.log(B.z), k)), cx: lerp(A.cx, B.cx, k), cy: lerp(A.cy, B.cy, k) });
  if (t < C + 0.15) return mix(K2, K3, E.io3(prog(t, FLASH.tab + 0.15, C - 0.25)));
  return mix(K3, K4, E.io3(prog(t, C + 0.15, C + 0.65)));
}
function cursorAt(path, t) {
  let i = 0;
  while (i < path.length - 1 && t > path[i + 1][0]) i++;
  const A = path[i], B = path[Math.min(i + 1, path.length - 1)];
  const k = B[0] > A[0] ? E.io3(prog(t, A[0], B[0])) : 1;
  return { x: lerp(A[1], B[1], k), y: lerp(A[2], B[2], k) };
}

function renderBrowser(t) {
  const y0 = browserY(t);
  const b = $('browser');
  b.style.top = y0 + 'px';
  const cur = $('cursor');
  cur.style.opacity = 0;
  const visible = y0 < 1090;
  b.style.visibility = visible ? 'visible' : 'hidden';
  if (!visible) return;
  const isEditor = t < S.anims;
  $('editor').style.visibility = isEditor ? 'visible' : 'hidden';
  $('site').style.visibility = isEditor ? 'hidden' : 'visible';
  const Wv = BROWSER.w - 6, Hv = BROWSER.h - 6 - 62;
  const ox = BROWSER.x + 3, oy = y0 + 3 + 62;

  if (isEditor) {
    const view = tt => { const c = editorCam(tt), z = c.z * editorBase; return { z, x: c.cx - Wv / 2 / z, y: c.cy - Hv / 2 / z }; };
    const v = view(t);
    $('editor').style.transform = `translate(${-v.x * v.z}px, ${-v.y * v.z}px) scale(${v.z})`;
    const st = editorState(t);
    const dialogIn = E.out3(prog(t, EDIT.gear + 0.1, EDIT.gear + 0.24));
    for (const img of document.querySelectorAll('#editor img')) {
      const s = img.dataset.state;
      img.style.opacity = s === st ? (s === 'dialog' ? dialogIn : 1) : (s === 'list' && st === 'dialog' ? 1 : 0);
    }
    // курсор идёт по кнопкам, координаты сняты с сайта
    const scr = (bx, tt) => { const q = view(tt); return [ox + (bx.x + bx.w * 0.55 - q.x) * q.z, oy + (bx.y + bx.h * 0.55 - q.y) * q.z]; };
    const g = scr(siteInfo.gear, EDIT.gear), zo = scr(siteInfo.zone, EDIT.zone), te = scr(siteInfo.option, EDIT.temp), dn = scr(siteInfo.done, EDIT.done);
    const path = [[S.editor + 0.56, 1150, 980], [EDIT.gear - 0.05, ...g], [EDIT.gear + 0.6, ...g], [EDIT.zone - 0.05, ...zo], [EDIT.temp - 0.4, ...zo], [EDIT.temp - 0.05, ...te],
      [EDIT.done - 0.4, ...te], [EDIT.done - 0.05, ...dn], [EDIT.done + 0.3, ...dn], [EDIT.done + 0.8, dn[0] + 220, dn[1] + 260]];
    const c = cursorAt(path, t);
    const press = [EDIT.gear, EDIT.zone, EDIT.temp, EDIT.done].some(tc => t >= tc && t < tc + 0.1);
    cur.style.opacity = prog(t, S.editor + 0.56, S.editor + 0.7) * (1 - prog(t, EDIT.done + 0.6, EDIT.done + 0.8));
    cur.style.transform = `translate(${c.x}px, ${c.y}px) scale(${press ? 0.86 : 1})`;
    return;
  }

  // прошивка: сначала вкладка USB-кабель, затем Bluetooth и прошивка
  setTab(t >= FLASH.tab);
  const cam = siteCam(t, Wv, Hv);
  const x = cam.cx - Wv / 2 / cam.z, y = cam.cy - Hv / 2 / cam.z;
  $('site').style.transform = `translate(${-x * cam.z}px, ${-y * cam.z}px) scale(${cam.z})`;
  const btn = $('s-flash-ble');
  const hover = t > C - 0.12 && t < C + 0.4;
  const press = t >= C && t < C + 0.12;
  btn.style.background = press || hover ? 'color-mix(in srgb, var(--ink) 84%, var(--paper))' : '';
  btn.style.transform = press ? 'scale(0.97)' : '';
  $('s-work').hidden = t < WORK[0].a;
  for (const w of WORK) {
    const st = t < w.a ? 'todo' : t < w.b ? 'active' : 'done';
    w.li.dataset.state = st;
    const box = w.li.querySelector('.w-box');
    box.style.transform = st === 'done' ? `scale(${lerp(1.35, 1, E.out3(prog(t, w.b, w.b + 0.22)))})` : '';
    box.style.backgroundSize = st === 'active' ? (Math.floor((t - w.a) / 0.5) % 2 ? '0 0' : '8px 8px') : '';
    const sec = 5 + Math.floor((Math.min(t, w.b) - C) * 20);
    w.li.querySelector('.w-time').textContent = st === 'done' ? `21:14:${String(sec).padStart(2, '0')}` : '';
  }
  const BYTES = siteInfo.firmware.bin;
  const kp = E.io3(prog(t, WORK[2].a + 0.05, WORK[2].b));
  $('s-progress-box').hidden = t < WORK[2].a;
  const bytes = kp >= 1 ? BYTES : Math.round(BYTES * kp / 244) * 244;
  const pct = Math.floor(bytes / BYTES * 100);
  $('s-progress').value = pct;
  $('s-progress-text').textContent = `${pct}%`;
  $('s-progress-bytes').textContent = `${fmt(bytes)} из ${fmt(BYTES)} байт`;
  let status = null;
  for (const s of STATUS) if (t >= s[0]) status = s;
  $('s-status').hidden = !status;
  if (status) { $('s-status').className = `status ${status[1]}`; $('s-status').textContent = status[2]; }
  $('fast').style.opacity = prog(t, C + 0.6, C + 0.8) * (1 - prog(t, C + 1.8, C + 2.0));

  // курсор: вкладка Bluetooth, затем кнопка
  const at = (bx, tt) => { const q = siteCam(tt, Wv, Hv), xx = q.cx - Wv / 2 / q.z, yy = q.cy - Hv / 2 / q.z; return [ox + (bx.x + bx.w * 0.5 - xx) * q.z, oy + (bx.y + bx.h * 0.55 - yy) * q.z]; };
  const tb = at(site.tab, FLASH.tab), bt = at({ ...site.btn, x: site.btn.x + site.btn.w * 0.12 }, C);
  const path = [[S.flash + 0.6, 1150, 980], [FLASH.tab - 0.05, ...tb], [FLASH.tab + 0.35, ...tb], [C - 0.05, ...bt], [C + 0.4, ...bt], [C + 0.9, bt[0] + 260, bt[1] + 220]];
  const c = cursorAt(path, t);
  const pressing = (t >= FLASH.tab && t < FLASH.tab + 0.1) || press;
  cur.style.opacity = prog(t, S.flash + 0.6, S.flash + 0.75) * (1 - prog(t, C + 0.7, C + 0.9));
  cur.style.transform = `translate(${c.x}px, ${c.y}px) scale(${pressing ? 0.86 : 1})`;
}

// ---------------------------------------------------------------- анимации
// петля по кругу: с конца логотипа (0,6 с записи) до end, чтобы плитки не застывали на данных
const TILES = [
  { name: 'wave', label: 'Волна', start: S.anims + 0.15, from: 600, end: 2700 },
  { name: 'break', label: 'Разрушение', start: S.anims + 0.35, from: 600, end: 3000 },
  { name: 'explosion', label: 'Взрыв', start: S.anims + 0.55, from: 600, end: 3500 },
  { name: 'poweroff', label: 'Выключение', start: S.anims + 0.2, from: 900, end: 2550 },
];
function initTiles() {
  const w = 512, h = 256, gap = 160;
  const x0 = (1920 - (2 * w + gap)) / 2;
  TILES.forEach((T, i) => {
    T.rect = { x: x0 + (i % 2) * (w + gap), y: i < 2 ? 282 : 650, w, h };
    T.frame = document.createElement('div'); T.frame.className = 'v-screen';
    T.cv = document.createElement('canvas');
    T.cap = document.createElement('p'); T.cap.className = 'v-tile-cap';
    T.cap.innerHTML = `<b>${i + 1}</b>${T.label}`;
    $('anims').append(T.frame, T.cv, T.cap);
    frameBox(T.frame, T.rect, 16);
    Object.assign(T.cap.style, { left: (T.rect.x - 19) + 'px', top: (T.rect.y + h + 33) + 'px' });
  });
}
function renderTiles(t) {
  for (const T of TILES) {
    const period = T.end - T.from;
    const ms = T.from + ((((t - T.start) * 1000) % period) + period) % period;
    drawOled(T.cv, T.rect, 16, sampleOled(T.name, ms));
  }
}

// ---------------------------------------------------------------- колонтитул: раздел и страница
const SECTIONS = [
  [S.cut, 'Устройство', 1], [S.link, 'Подключение', 2], [S.open, 'Исходники и сборка', 3], [S.screen, 'Основной экран', 4],
  [S.alarm, 'Предупреждение о ШИМ', 4], [S.editor, 'Настройка экранов', 5], [S.anims, 'Анимации', 6], [S.flash, 'Прошивка', 7], [S.outro, 'Где взять', 8],
];

// ---------------------------------------------------------------- кадр
function render(t) {
  // вступление в 3D
  const use3d = t < S.cut;
  $('three').style.visibility = use3d ? 'visible' : 'hidden';
  if (use3d) {
    const st = state3d(t);
    paintScreen(st.screen);
    dev.render(st);
  }

  // лист
  const pageOn = t >= S.cut && (t < S.editor + 0.6 || t >= S.outro);
  const page = $('page'), cv = $('oled-main');
  page.style.display = $('page-top').style.display = pageOn ? '' : 'none';
  cv.style.visibility = pageOn ? 'visible' : 'hidden';
  if (pageOn) {
    const cam = pageCam(t);
    const fadeOut = 1 - E.in3(prog(t, S.editor + 0.05, S.editor + 0.45));
    const top = $('page-top');
    page.style.transform = top.style.transform = `translate(${960 - cam.cx * cam.s}px, ${540 - cam.cy * cam.s}px) scale(${cam.s})`;
    page.style.opacity = top.style.opacity = t < S.outro ? fadeOut : 1;

    const f = figState(t), pad = padOf(f.r);
    frameBox($('fig'), f.r, pad);
    $('fig').style.opacity = f.o;
    const tl = toScreen(cam, f.r.x, f.r.y);
    const [scene, ms] = figSource(t);
    drawOled(cv, { x: tl.x, y: tl.y, w: f.r.w * cam.s, h: f.r.h * cam.s }, pad * cam.s, sampleOled(scene, ms));
    cv.style.opacity = f.o * (t < S.outro ? fadeOut : 1);

    // подпись рисунка: титул и концовка
    const cap = $('fig-cap');
    const inTitle = t < S.link;
    cap.innerHTML = inTitle ? '<b>Рис. 1.</b> Экран IMBA TV, 128×64 точки' : '<b>Рис. 6.</b> Выключается, как старый телевизор';
    Object.assign(cap.style, { left: (f.r.x - pad - 3) + 'px', top: (f.r.y + f.r.h + pad + 3 + 34) + 'px' });
    const kc = inTitle ? inOut(t, S.cut + 0.7, 0.4, S.link, 0.2).o : (t >= S.outro ? E.out3(prog(t, CRT + 0.5, CRT + 0.9)) : 0);
    show(cap, kc);

    // титул
    const kt = inOut(t, S.cut + 0.45, 0.5, S.link, 0.22);
    show($('title'), 1 - kt.kout);
    [['.v-h1', 0], ['.v-lede', 0.15], ['.v-sub', 0.3]].forEach(([sel, d]) => {
      const k = E.out3(prog(t, S.cut + 0.45 + d, S.cut + 0.95 + d));
      show($('title').querySelector(sel), k * (1 - kt.kout), 0, 40 * (1 - k));
    });

    // подключение
    const kl = inOut(t, S.link + 0.45, 0.4, S.open, 0.22);
    show($('link'), kl.o, 0, 20 * (1 - kl.kin));
    renderLink(t);

    // открытый проект
    const ko = inOut(t, S.open + 0.25, 0.4, S.screen, 0.22);
    show($('open'), ko.o);
    $('open').querySelectorAll('.v-row').forEach((r, i) => {
      const k = E.out3(prog(t, S.open + 0.45 + i * 0.18, S.open + 0.85 + i * 0.18));
      r.style.opacity = k; r.style.transform = `translateY(${24 * (1 - k)}px)`;
    });
    const kr = E.out3(prog(t, S.open + 0.55, S.open + 0.95));
    $('open').querySelector('.v-repo').style.transform = `translateX(${40 * (1 - kr)}px)`;
    $('open').querySelector('.v-repo').style.opacity = kr;
    $('open').querySelectorAll('.v-repo-list li').forEach((li, i) => {
      li.style.opacity = E.out3(prog(t, S.open + 0.75 + i * 0.09, S.open + 1.05 + i * 0.09));
    });

    // экран и тревога по ШИМ
    const kh = inOut(t, S.screen + 0.4, 0.4, S.alarm, 0.2);
    show($('screen-head'), kh.o, 0, 30 * (1 - kh.kin));
    renderLeaders(t);
    const c4 = inOut(t, S.alarm + 0.3, 0.45, S.editor, 0.3);
    const callout = $('alarm-callout');
    Object.assign(callout.style, { left: (1120 / 2.35) + 'px', top: (262 / 2.35) + 'px' });
    show(callout, c4.o, 40 * (1 - c4.kin) / 2.35, 0);
    const ch = inOut(t, S.alarm + 0.2, 0.4, S.editor, 0.3);
    show($('alarm-chart'), ch.o, 0, 24 * (1 - ch.kin));
    renderChart(t);

    // концовка
    const ke = t >= S.outro;
    show($('end'), ke ? 1 : 0);
    $('end').querySelectorAll('.v-h1, .v-row').forEach((el, i) => {
      const k = E.out3(prog(t, S.outro + 0.6 + i * 0.16, S.outro + 1.05 + i * 0.16));
      el.style.opacity = k; el.style.transform = `translateY(${30 * (1 - k)}px)`;
    });
  }

  // браузер
  renderBrowser(t);

  // правая колонка рядом с браузером
  const inEditor = t >= S.editor && t < S.anims;
  const inFlash = t >= S.flash && t < S.outro + 0.5;
  const side = $('side');
  if (inEditor || inFlash) {
    const t0 = inEditor ? S.editor : S.flash;
    const kout = inEditor ? E.in3(prog(t, S.anims - 0.2, S.anims)) : E.in3(prog(t, S.outro, S.outro + 0.35));
    const ks = E.out3(prog(t, t0 + 0.3, t0 + 0.75));
    $('side-head').textContent = inEditor ? 'Экраны под себя' : 'Прошивка из браузера';
    $('side-sub').textContent = inEditor ? 'До пяти экранов, любые значения, в любом порядке' : 'По USB-C или по Bluetooth, без\u00a0Arduino\u00a0IDE';
    show(side, ks * (1 - kout), 0, 20 * (1 - ks));
    const ki = E.out5(prog(t, t0 + 0.4, t0 + 0.95));
    const IR = { x: lerp(1316 + 720, 1316, ki) + 720 * kout, y: 432, w: 512, h: 256 };
    const ipad = padOf(IR);
    frameBox($('inset'), IR, ipad);
    $('inset').style.visibility = $('oled-inset').style.visibility = 'visible';
    const icap = $('inset-cap');
    Object.assign(icap.style, { left: (IR.x - ipad - 3) + 'px', top: (IR.y + IR.h + ipad + 3 + 30) + 'px' });
    let br, capText, kc;
    if (inEditor) {
      br = t < EDIT.done + 0.2 ? sampleOled('ride', 0) : sampleOled('cycle', (t - (EDIT.done + 0.2)) * 1000);
      capText = '<b>Рис. 7.</b> После прошивки экраны переключаются кнопкой';
      kc = E.out3(prog(t, EDIT.done + 0.2, EDIT.done + 0.6));
    } else if (t < FLASH.tab) {
      br = sampleOled('charging', (t - S.flash) * 1000 + 300);
      capText = '<b>Рис. 8.</b> По USB-C идут зарядка и&nbsp;прошивка';
      kc = E.out3(prog(t, S.flash + 0.7, S.flash + 1.0)) * (1 - prog(t, FLASH.tab - 0.15, FLASH.tab));
    } else {
      br = t < FLASH.tab + 0.15 ? null : sampleOled('update', (t - (C + 0.25 - 3.0)) * 1000);
      capText = '<b>Рис. 8.</b> Дисплей в&nbsp;режиме обновления по&nbsp;Bluetooth';
      kc = E.out3(prog(t, FLASH.tab + 0.1, FLASH.tab + 0.4));
    }
    icap.innerHTML = capText;
    show(icap, kc * (1 - kout), 0, 12 * (1 - kc));
    drawOled($('oled-inset'), IR, ipad, br);
  } else {
    show(side, 0);
    $('inset').style.visibility = $('oled-inset').style.visibility = $('inset-cap').style.visibility = 'hidden';
  }

  // анимации
  const animsOn = t >= S.anims - 0.01 && t < S.flash + 0.55;
  $('anims').style.visibility = animsOn ? 'visible' : 'hidden';
  if (animsOn) {
    $('anims').style.opacity = 1 - E.in3(prog(t, S.flash + 0.1, S.flash + 0.5));
    renderTiles(t);
  }

  // колонтитул и подвал
  let sec = SECTIONS[0];
  for (const s of SECTIONS) if (t >= s[0]) sec = s;
  $('rh-sec').textContent = sec[1];
  $('page-n').textContent = sec[2];
  const chrome = E.out3(prog(t, S.cut + 0.4, S.cut + 0.8));
  show($('runhead'), chrome);
  const footHidden = (t >= S.editor + 0.05 && t < S.anims) || (t >= S.flash + 0.05 && t < S.outro + 0.3);
  show($('foot'), footHidden ? 0 : chrome);
  const swap = SECTIONS.map(s => Math.abs(t - s[0] - 0.05)).reduce((a, c) => Math.min(a, c));
  $('rh-sec').style.opacity = clamp(swap / 0.12);

  // переход через чистый лист к анимациям
  const d = S.anims;
  $('dip').style.opacity = t < d ? E.in3(prog(t, d - 0.16, d)) : 1 - E.out3(prog(t, d, d + 0.22));
}

// ---------------------------------------------------------------- запуск
window.DURATION = DURATION;
window.SCENES = { ...S, EDIT, FLASH, CRT };
window.renderFrame = async t => {
  render(t);
  await new Promise(r => requestAnimationFrame(() => requestAnimationFrame(r)));
  return true;
};
window.__ready = (async () => {
  await initSite();
  await document.fonts.ready;
  await Promise.all(['800 40px "Sofia Sans Condensed"', '700 20px "Sofia Sans Condensed"', '400 20px "Sofia Sans"', '600 20px "Sofia Sans"', '500 20px "Sofia Sans"']
    .map(f => document.fonts.load(f, 'ШИМ скорость IMBA')));
  dev = createDevice($('three'), tex);
  initLeaders();
  initChart();
  initLink();
  initTiles();
  measureSite();
  render(0);
  return true;
})();
