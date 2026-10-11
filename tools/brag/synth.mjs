// Музыка и звуки для видео IMBA TV: 125 BPM, ля минор. Всё синтезируется здесь, на выходе WAV 48 кГц.
// Эффекты в той же тональности и в том же ревербераторе, что и музыка, и стоят ниже неё в миксе.
// Запуск: node synth.mjs out.wav timeline.json (времена сцен из композиции и мигания экрана из стенда)
import fs from 'node:fs';

const TL = JSON.parse(fs.readFileSync(process.argv[3], 'utf8'));
const { S, EDIT, FLASH, CRT } = TL;
const SR = 48000;
const DUR = TL.duration;
const N = Math.ceil(DUR * SR);
const BEAT = 0.48, BAR = 1.92, S16 = 0.12;
const mtof = m => 440 * Math.pow(2, (m - 69) / 12);

// детерминированный шум
let seed = 0x1a2b3c4d;
const rnd = () => { seed = (seed + 0x6D2B79F5) | 0; let t = Math.imul(seed ^ (seed >>> 15), 1 | seed); t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296 * 2 - 1; };

class Bus {
  constructor() { this.L = new Float32Array(N); this.R = new Float32Array(N); }
  add(i, v, pan = 0) {
    if (i < 0 || i >= N) return;
    const a = (pan + 1) * Math.PI / 4;
    this.L[i] += v * Math.cos(a);
    this.R[i] += v * Math.sin(a);
  }
}

class Biquad {
  constructor() { this.x1 = this.x2 = this.y1 = this.y2 = 0; this.set('lp', 1000, 0.707); }
  set(type, f, q) {
    f = Math.min(Math.max(f, 10), SR * 0.45);
    const w = 2 * Math.PI * f / SR, c = Math.cos(w), s = Math.sin(w), al = s / (2 * q);
    let b0, b1, b2, a0 = 1 + al, a1 = -2 * c, a2 = 1 - al;
    if (type === 'lp') { b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2; }
    else if (type === 'hp') { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; }
    else { b0 = al; b1 = 0; b2 = -al; }  // bp
    this.b0 = b0 / a0; this.b1 = b1 / a0; this.b2 = b2 / a0; this.a1 = a1 / a0; this.a2 = a2 / a0;
  }
  p(x) {
    const y = this.b0 * x + this.b1 * this.x1 + this.b2 * this.x2 - this.a1 * this.y1 - this.a2 * this.y2;
    this.x2 = this.x1; this.x1 = x; this.y2 = this.y1; this.y1 = y;
    return y;
  }
}

const blep = (t, dt) => {
  if (t < dt) { t /= dt; return t + t - t * t - 1; }
  if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
  return 0;
};
const saw = (ph, dt) => 2 * ph - 1 - blep(ph, dt);
const pulse = (ph, dt, pw) => {
  let v = ph < pw ? 1 : -1;
  v += blep(ph, dt);
  let p2 = ph - pw; if (p2 < 0) p2 += 1;
  v -= blep(p2, dt);
  return v;
};

const music = new Bus();     // всё, что тормозит как плёнка
const musicVerb = new Bus(); // посыл музыки в ревербератор
const sfx = new Bus();
const sfxVerb = new Bus();
const delayIn = new Bus();
const endBus = new Bus();

// ---------------------------------------------------------------- ударные

const kicks = [];
function kick(t0, g = 0.9) {
  kicks.push(t0);
  const i0 = Math.round(t0 * SR);
  let ph = 0;
  const lp = new Biquad(); lp.set('lp', 3500, 0.7);
  for (let n = 0; n < 0.5 * SR; n++) {
    const t = n / SR;
    const f = 55 + 80 * Math.exp(-t / 0.028);
    ph += f / SR;
    const amp = Math.exp(-t / 0.17) * Math.min(1, t / 0.0015);
    const click = lp.p(rnd()) * Math.exp(-t / 0.003) * 0.25;
    music.add(i0 + n, (Math.sin(2 * Math.PI * ph) * amp + click) * g);
  }
}

function clap(t0, g = 0.32) {
  const i0 = Math.round(t0 * SR);
  const bp = new Biquad(); bp.set('bp', 1700, 0.75);
  const hp = new Biquad(); hp.set('hp', 750, 0.7);
  for (let n = 0; n < 0.3 * SR; n++) {
    const t = n / SR;
    let env = 0;
    for (const o of [0, 0.010, 0.021]) if (t >= o) env = Math.max(env, Math.exp(-(t - o) / 0.0055));
    if (t >= 0.028) env = Math.max(env, 0.55 * Math.exp(-(t - 0.028) / 0.11));
    const v = hp.p(bp.p(rnd())) * env * g * 1.6;
    music.add(i0 + n, v, 0.05);
    musicVerb.add(i0 + n, v * 0.45);
  }
}

function hat(t0, g = 0.07, open = false, pan = 0.2) {
  const i0 = Math.round(t0 * SR);
  const hp = new Biquad(); hp.set('hp', 7600, 0.8);
  const lp = new Biquad(); lp.set('lp', 13000, 0.7);
  const tau = open ? 0.11 : 0.034;
  for (let n = 0; n < tau * 6 * SR; n++) {
    const t = n / SR;
    music.add(i0 + n, lp.p(hp.p(rnd())) * Math.exp(-t / tau) * g, pan);
  }
}

// посыл музыки: приглушение под бочку
function duck(t) {
  let d = 1;
  for (let k = kicks.length - 1; k >= 0; k--) {
    const dt = t - kicks[k];
    if (dt < 0) continue;
    if (dt > 0.4) break;
    d = Math.min(d, 1 - 0.55 * Math.exp(-dt / 0.11) * Math.min(1, dt / 0.004 + 0.3));
  }
  return d;
}

// ---------------------------------------------------------------- тональные

function bass(m, t0, dur, g = 0.2) {
  const i0 = Math.round(t0 * SR), f = mtof(m), dt = f / SR;
  let ph = rnd() * 0.5 + 0.5, ph2 = 0;
  const lp = new Biquad();
  const len = (dur + 0.04) * SR;
  for (let n = 0; n < len; n++) {
    const t = n / SR;
    if (n % 32 === 0) lp.set('lp', 420 + 1500 * Math.exp(-t / 0.07), 0.9);
    ph += dt; if (ph >= 1) ph -= 1;
    ph2 += dt; if (ph2 >= 1) ph2 -= 1;
    const env = Math.min(1, t / 0.004) * (t > dur ? Math.exp(-(t - dur) / 0.012) : 1);
    const v = (lp.p(saw(ph, dt)) * 0.75 + Math.sin(2 * Math.PI * ph2) * 0.42) * env * g;
    music.add(i0 + n, v);
  }
}

// пэды пишем отдельно, потом приглушаем под бочку
const padBus = new Bus();
function pad(notes, t0, dur, g = 0.03, cutoff = 1500, att = 0.35) {
  const i0 = Math.round(t0 * SR);
  for (const m of notes) {
    for (const [cents, pan] of [[-8, -0.6], [0, 0], [7, 0.6]]) {
      const f = mtof(m) * Math.pow(2, cents / 1200), dt = f / SR;
      let ph = (rnd() + 1) / 2;
      const lp = new Biquad(); lp.set('lp', cutoff, 0.6);
      const len = (dur + 0.8) * SR;
      for (let n = 0; n < len; n++) {
        const t = n / SR;
        ph += dt; if (ph >= 1) ph -= 1;
        const env = Math.min(1, t / att) * (t > dur ? Math.exp(-(t - dur) / 0.35) : 1);
        padBus.add(i0 + n, lp.p(saw(ph, dt)) * env * g, pan);
      }
    }
  }
}

function pluck(m, t0, g = 0.05, pan = 0, bright = 1) {
  const i0 = Math.round(t0 * SR), f = mtof(m), dt = f / SR;
  let ph = 0;
  const lp = new Biquad();
  for (let n = 0; n < 0.45 * SR; n++) {
    const t = n / SR;
    if (n % 32 === 0) lp.set('lp', (1300 + 4200 * Math.exp(-t / 0.06)) * bright, 1.0);
    ph += dt; if (ph >= 1) ph -= 1;
    const env = Math.min(1, t / 0.002) * Math.exp(-t / 0.12);
    const v = lp.p(pulse(ph, dt, 0.25)) * env * g;
    music.add(i0 + n, v, pan);
    delayIn.add(i0 + n, v * 0.5, pan);
    musicVerb.add(i0 + n, v * 0.3);
  }
}

function bell(m, t0, g, decay = 1.3, bus = sfx, verb = sfxVerb, pan = 0) {
  const i0 = Math.round(t0 * SR), f = mtof(m);
  for (let n = 0; n < decay * 4 * SR; n++) {
    const t = n / SR;
    const idx = 0.25 + 2.2 * Math.exp(-t / 0.25);
    const mod = Math.sin(2 * Math.PI * f * 3.5 * t) * idx;
    const v = (Math.sin(2 * Math.PI * f * t + mod) * 0.8 + Math.sin(2 * Math.PI * f * 2 * t) * 0.15 * Math.exp(-t / 0.4))
      * Math.exp(-t / decay) * Math.min(1, t / 0.003) * g;
    bus.add(i0 + n, v, pan);
    verb.add(i0 + n, v * 0.6);
  }
}

// ---------------------------------------------------------------- звуки сцен

function noiseSweep(t0, dur, f0, f1, g, { q = 1.2, pan = 0, verb = 0.4, shape = 'swell' } = {}) {
  const i0 = Math.round(t0 * SR);
  const bp = new Biquad();
  for (let n = 0; n < dur * SR; n++) {
    const k = n / (dur * SR);
    if (n % 32 === 0) bp.set('bp', f0 * Math.pow(f1 / f0, k), q);
    const env = shape === 'swell' ? Math.sin(Math.PI * Math.min(1, k)) ** 2 : shape === 'rise' ? k * k : (1 - k) ** 2;
    const v = bp.p(rnd()) * env * g;
    sfx.add(i0 + n, v, pan);
    sfxVerb.add(i0 + n, v * verb);
  }
}

function boom(t0, g = 1) {
  const i0 = Math.round(t0 * SR);
  let ph = 0;
  const lp = new Biquad();
  for (let n = 0; n < 1.8 * SR; n++) {
    const t = n / SR;
    const f = 41 + 70 * Math.exp(-t / 0.09);
    ph += f / SR;
    const sub = Math.sin(2 * Math.PI * ph) * Math.exp(-t / 0.32) * Math.min(1, t / 0.002) * 0.7;
    if (n % 32 === 0) lp.set('lp', 250 + 4200 * Math.exp(-t / 0.13), 0.7);
    const rumble = lp.p(rnd()) * Math.exp(-t / 0.3) * 0.7;
    const v = (sub + rumble) * g;
    sfx.add(i0 + n, v);
    sfxVerb.add(i0 + n, rumble * g * 0.5);
  }
  // треск разлетающихся точек: короткие щелчки, реже со временем
  const bp = new Biquad(); bp.set('bp', 2400, 1.4);
  for (let k = 0; k < 70; k++) {
    const tt = t0 + 0.04 + Math.pow((rnd() + 1) / 2, 2.2) * 1.3;
    const j0 = Math.round(tt * SR), pan = rnd() * 0.8;
    const amp = 0.09 * g * (1 - (tt - t0) / 1.5);
    for (let n = 0; n < 0.006 * SR; n++) {
      const v = bp.p(Math.sign(rnd())) * Math.exp(-n / SR / 0.0015) * amp;
      sfx.add(j0 + n, v, pan);
      sfxVerb.add(j0 + n, v * 0.5);
    }
  }
}

function blip(m, t0, g, wave = 'sine', len = 0.09, pan = 0) {
  const i0 = Math.round(t0 * SR), f = mtof(m), dt = f / SR;
  let ph = 0;
  const lp = new Biquad(); lp.set('lp', 4200, 0.7);
  for (let n = 0; n < len * 5 * SR; n++) {
    const t = n / SR;
    ph += dt; if (ph >= 1) ph -= 1;
    const osc = wave === 'sine' ? Math.sin(2 * Math.PI * ph) : lp.p(pulse(ph, dt, 0.5)) * 0.5;
    const v = osc * Math.min(1, t / 0.002) * Math.exp(-t / len) * g;
    sfx.add(i0 + n, v, pan);
    sfxVerb.add(i0 + n, v * 0.45);
  }
}

function click(t0, g = 0.12) {
  const i0 = Math.round(t0 * SR);
  const bp = new Biquad(); bp.set('bp', 3200, 1.0);
  for (let n = 0; n < 0.03 * SR; n++) {
    const t = n / SR;
    const v = (bp.p(rnd()) * 1.4 + Math.sin(2 * Math.PI * 1760 * t) * 0.4) * Math.exp(-t / 0.004) * g;
    sfx.add(i0 + n, v, 0.15);
    sfxVerb.add(i0 + n, v * 0.3);
  }
}

function stamp(m, t0, g = 0.1) {
  const i0 = Math.round(t0 * SR), f = mtof(m);
  const bp = new Biquad(); bp.set('bp', f * 2, 2);
  for (let n = 0; n < 0.15 * SR; n++) {
    const t = n / SR;
    const v = (Math.sin(2 * Math.PI * f * t) * Math.exp(-t / 0.045) + bp.p(rnd()) * Math.exp(-t / 0.008) * 0.6) * g;
    sfx.add(i0 + n, v, -0.1);
    sfxVerb.add(i0 + n, v * 0.35);
  }
}

// выключение ЭЛТ: тон падает вниз, лёгкий треск, щелчок в конце
function crt(t0, g = 0.16) {
  const i0 = Math.round(t0 * SR);
  let ph = 0;
  const hp = new Biquad(); hp.set('hp', 2500, 0.7);
  for (let n = 0; n < 0.6 * SR; n++) {
    const t = n / SR;
    const f = 60 + 1700 * Math.exp(-t / 0.11);
    ph += f / SR;
    const env = Math.min(1, t / 0.004) * Math.exp(-t / 0.2);
    const v = (Math.sin(2 * Math.PI * ph) * 0.8 + hp.p(rnd()) * 0.25 * Math.exp(-t / 0.06)) * env * g;
    sfx.add(i0 + n, v);
    sfxVerb.add(i0 + n, v * 0.5);
  }
}

// ---------------------------------------------------------------- аранжировка

const CH = {
  Am9: { bass: 45, pad: [57, 60, 64, 67, 71], arp: [69, 72, 76, 79, 83, 79, 76, 72] },
  Fmaj9: { bass: 41, pad: [57, 60, 64, 67], arp: [65, 69, 72, 76, 79, 76, 72, 69] },
  Cmaj7: { bass: 48, pad: [55, 59, 64, 67], arp: [67, 72, 76, 79, 83, 79, 76, 72] },
  G6: { bass: 43, pad: [59, 62, 64, 67], arp: [67, 71, 74, 76, 79, 76, 74, 71] },
};
const PROG = ['Am9', 'Fmaj9', 'Cmaj7', 'G6'];
const STOP = CRT;                // плёнка тормозит вместе с выключением экрана
const GROOVE = S.cut;            // такты начинаются со склейки: экран становится листом
const DROPS = [S.screen, S.editor, S.flash];   // перед ними подъём и пропуск бочки

// вступление: пэд раскрывается, пока корпус летит и включается
pad(CH.Am9.pad, 0, GROOVE - 0.05, 0.036, 1500, 1.4);
for (let b = 0; GROOVE + b * BAR < STOP; b++) {
  const t0 = GROOVE + b * BAR, ch = CH[PROG[b % 4]];
  const late = t0 >= S.editor - 0.01;
  const tense = t0 >= S.screen - 0.01;
  pad(ch.pad, t0, BAR - 0.05, 0.04, late ? 4200 : 3400, 0.08);
  const BP = [[0, 2, 0], [3, 1, 0], [6, 1, 12], [8, 2, 0], [11, 1, 0], [14, 1, 7]];
  for (const [st, len, iv] of BP) {
    const t = t0 + st * S16;
    if (t < STOP) bass(ch.bass + iv, t, len * S16 * 0.85, late ? 0.21 : 0.19);
  }
  for (let q = 0; q < 4; q++) {
    const t = t0 + q * BEAT;
    if (t >= STOP) continue;
    if (!DROPS.some(d => Math.abs(t - (d - BEAT)) < 0.01)) kick(t, 0.62);
    if (q % 2 === 1) clap(t, late ? 0.5 : 0.42);
    hat(t + BEAT / 2, 0.19, q === 3, 0.25);
    if (tense) { hat(t + S16, 0.075, false, -0.25); hat(t + 3 * S16, 0.09, false, -0.25); }
  }
  for (let st = 0; st < 16; st++) {
    const t = t0 + st * S16;
    if (t >= STOP) continue;
    const m = ch.arp[st % 8] + (late && st % 4 === 2 ? 12 : 0);
    pluck(m, t, (late ? 0.1 : 0.085) * (st % 4 === 0 ? 1 : 0.65), st % 2 ? 0.35 : -0.35, late ? 1.45 : 1.25);
  }
}
for (const d of [GROOVE, ...DROPS]) noiseSweep(d - BAR / 2, BAR / 2, 400, 6000, 0.045, { shape: 'rise', q: 0.9, verb: 0.3 });

// ---------------------------------------------------------------- события картинки

// корпус влетает, садится, кнопка, включение, взрыв, камера уходит в экран
noiseSweep(0.0, S.land, 500, 2200, 0.04, { shape: 'swell', q: 0.7, verb: 0.4 });
(function thud(t0) {
  const i0 = Math.round(t0 * SR);
  const lp = new Biquad(); lp.set('lp', 900, 0.7);
  let ph = 0;
  for (let n = 0; n < 0.25 * SR; n++) {
    const t = n / SR;
    ph += (70 + 60 * Math.exp(-t / 0.02)) / SR;
    const v = (Math.sin(2 * Math.PI * ph) * 0.6 + lp.p(rnd()) * 0.35) * Math.exp(-t / 0.05) * 0.22;
    sfx.add(i0 + n, v);
    sfxVerb.add(i0 + n, v * 0.3);
  }
})(S.land);
click(S.land + 0.04, 0.09);
bell(81, S.on, 0.03, 0.7);
bell(88, S.on + 0.05, 0.02, 0.7);
boom(S.boom, 0.9);
noiseSweep(GROOVE, 0.7, 3200, 500, 0.04, { shape: 'fall', q: 0.9, verb: 0.5 });

// подключение: пакет дошёл до телефона, затем до дисплея
blip(84, S.link + 1.6, 0.035, 'sine', 0.06, -0.2);
bell(84, S.link + 2.45, 0.035, 0.8);
bell(88, S.link + 2.5, 0.025, 0.8);

// открытый проект: строки репозитория
for (let i = 0; i < 7; i++) blip(93 + (i % 3) * 2, S.open + 0.75 + i * 0.09, 0.012, 'sine', 0.03, (i - 3) * 0.1);

// выноски
[81, 84, 88, 93].forEach((m, i) => blip(m, S.screen + 0.55 + i * 0.2, 0.05, 'sine', 0.06, i % 2 ? 0.3 : -0.3));

// мигания экрана: высота растёт по пентатонике
const BLINK_NOTES = [76, 79, 81, 84, 86, 88, 91];
TL.blinks.map(ms => S.alarm + ms / 1000).filter(t => t < S.editor + 0.45).forEach((t, i) => {
  const g = t > S.editor ? 0.035 : 0.07;
  blip(BLINK_NOTES[Math.min(i, 6)], t, g, 'square', 0.07);
  blip(BLINK_NOTES[Math.min(i, 6)] + 12, t, g * 0.35, 'sine', 0.05);
});

// редактор экранов: окно въезжает, четыре нажатия, затем кнопка на дисплее
noiseSweep(S.editor, 0.6, 2500, 600, 0.04, { shape: 'swell', q: 0.8 });
[EDIT.gear, EDIT.zone, EDIT.temp, EDIT.done].forEach(t => click(t, 0.12));
for (let i = 1; i <= 3; i++) { click(EDIT.done + 0.2 + 0.2 + 0.7 * i, 0.06); blip(88 + i * 2, EDIT.done + 0.2 + 0.2 + 0.7 * i, 0.02, 'sine', 0.04); }

// анимации: переход, волна, трещины, взрыв, выключения
noiseSweep(S.anims - 0.16, 0.4, 3000, 900, 0.03, { shape: 'swell', q: 0.8 });
// моменты эффектов те же, что у плиток в композиции: старт + (1000 - 600) мс, затем по кругу
for (let k = 0; k < 2; k++) {
  noiseSweep(S.anims + 0.55 + k * 2.1, 0.9, 300, 1500, 0.025, { shape: 'swell', q: 1.2, verb: 0.5 });
  for (let j = 0; j < 6; j++) click(S.anims + 0.75 + k * 2.4 + j * 0.06, 0.025);
  if (S.anims + 0.95 + k * 2.9 < S.flash - 0.1) boom(S.anims + 0.95 + k * 2.9, 0.2);
}
for (const t of [S.anims + 0.8, S.anims + 2.45]) crt(t, 0.05);

// прошивка: окно въезжает, вкладка, кнопка, этапы, готово
noiseSweep(S.flash, 0.6, 2500, 600, 0.04, { shape: 'swell', q: 0.8 });
click(FLASH.tab, 0.12);
click(FLASH.click, 0.14);
[[0.3, 76], [0.54, 81], [1.75, 84], [2.0, 88]].forEach(([d, m]) => stamp(m, FLASH.click + d, 0.09));
[[0, 81], [0.04, 84], [0.08, 88], [0.12, 95]].forEach(([d, m], i) => bell(m, FLASH.click + 2.0 + d, 0.045, 0.9, sfx, sfxVerb, (i - 1.5) * 0.25));

// концовка: окно уезжает, экран выключается как ЭЛТ, финальный аккорд
noiseSweep(S.outro, 0.5, 600, 2800, 0.03, { shape: 'swell', q: 0.8 });
crt(STOP, 0.16);
click(STOP + 0.45, 0.06);
const END = STOP + BEAT;
[[57, 0], [64, 0.035], [69, 0.07], [72, 0.105], [76, 0.14], [83, 0.2]].forEach(([m, d], i) => bell(m, END + d, 0.05, 1.6, endBus, sfxVerb, (i - 2.5) * 0.2));
(() => {
  const i0 = Math.round(END * SR);
  for (const m of [57, 60, 64, 67, 71]) {
    for (const [cents, pan] of [[-7, -0.5], [6, 0.5]]) {
      const f = mtof(m) * Math.pow(2, cents / 1200), dt = f / SR;
      let ph = (rnd() + 1) / 2;
      const lp = new Biquad(); lp.set('lp', 1300, 0.6);
      for (let n = i0; n < N; n++) {
        const t = (n - i0) / SR;
        ph += dt; if (ph >= 1) ph -= 1;
        const v = lp.p(saw(ph, dt)) * Math.min(1, t / 0.25) * Math.exp(-t / 2.2) * 0.02;
        endBus.add(n, v, pan);
        sfxVerb.add(n, v * 0.5);
      }
    }
  }
  let ph = 0;
  for (let n = i0; n < N; n++) {
    const t = (n - i0) / SR;
    ph += mtof(33) / SR;
    endBus.add(n, Math.sin(2 * Math.PI * ph) * Math.min(1, t / 0.02) * Math.exp(-t / 1.1) * 0.12);
  }
})();

// ---------------------------------------------------------------- обработка

// пэд под бочку
for (let n = 0; n < N; n++) {
  const d = duck(n / SR);
  music.L[n] += padBus.L[n] * d;
  music.R[n] += padBus.R[n] * d;
  musicVerb.L[n] += padBus.L[n] * d * 0.35;
  musicVerb.R[n] += padBus.R[n] * d * 0.35;
}

// пинг-понг 3/16 для арпеджио
(() => {
  const D = Math.round(0.36 * SR);
  const bufL = new Float32Array(N), bufR = new Float32Array(N);
  const lpL = new Biquad(), lpR = new Biquad(); lpL.set('lp', 3000, 0.7); lpR.set('lp', 3000, 0.7);
  for (let n = 0; n < N; n++) {
    const dl = n >= D ? bufR[n - D] : 0, dr = n >= D ? bufL[n - D] : 0;
    bufL[n] = delayIn.L[n] + lpL.p(dl) * 0.38;
    bufR[n] = delayIn.R[n] + lpR.p(dr) * 0.38;
    music.L[n] += dl * 0.45;
    music.R[n] += dr * 0.45;
  }
})();

// Freeverb: одна и та же комната для музыки и эффектов
function freeverb(src, wet = 1) {
  const sc = SR / 44100;
  const combs = [1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617].map(x => Math.round(x * sc));
  const aps = [556, 441, 341, 225].map(x => Math.round(x * sc));
  const out = new Bus();
  for (const [ch, spread] of [['L', 0], ['R', Math.round(23 * sc)]]) {
    const cb = combs.map(l => ({ buf: new Float32Array(l + spread), i: 0, f: 0 }));
    const ab = aps.map(l => ({ buf: new Float32Array(l + spread), i: 0 }));
    const o = out[ch];
    for (let n = 0; n < N; n++) {
      const x = (src.L[n] + src.R[n]) * 0.5 * 0.015 * 2;
      let y = 0;
      for (const c of cb) {
        const v = c.buf[c.i];
        c.f = v * 0.7 + c.f * 0.3;
        c.buf[c.i] = x + c.f * 0.84;
        if (++c.i >= c.buf.length) c.i = 0;
        y += v;
      }
      for (const a of ab) {
        const v = a.buf[a.i];
        a.buf[a.i] = y + v * 0.5;
        if (++a.i >= a.buf.length) a.i = 0;
        y = v - y;
      }
      o[n] = y * wet;
    }
  }
  return out;
}
const mv = freeverb(musicVerb, 0.9);
for (let n = 0; n < N; n++) { music.L[n] += mv.L[n]; music.R[n] += mv.R[n]; }

// плёнка тормозит: скорость падает до нуля за 0,42 с
(() => {
  const D = 0.42, s0 = Math.round(STOP * SR);
  const L = music.L.slice(), R = music.R.slice();
  for (let n = s0; n < N; n++) {
    const tau = (n - s0) / SR;
    if (tau >= D) { music.L[n] = music.R[n] = 0; continue; }
    const pos = STOP + D / 3 * (1 - Math.pow(1 - tau / D, 3));
    const x = pos * SR, i = Math.floor(x), fr = x - i;
    const fade = Math.pow(1 - tau / D, 0.7);
    music.L[n] = (L[i] * (1 - fr) + L[i + 1] * fr) * fade;
    music.R[n] = (R[i] * (1 - fr) + R[i + 1] * fr) * fade;
  }
})();

const sv = freeverb(sfxVerb, 0.9);

// сведение
const outL = new Float32Array(N), outR = new Float32Array(N);
const hpL = new Biquad(), hpR = new Biquad(); hpL.set('hp', 28, 0.7); hpR.set('hp', 28, 0.7);
let peak = 0;
for (let n = 0; n < N; n++) {
  const t = n / SR;
  const fadeIn = Math.min(1, t / 0.01);
  const fadeOut = Math.min(1, (DUR - t) / 0.6);
  let l = (music.L[n] * 0.95 + sfx.L[n] * 0.8 + sv.L[n] + endBus.L[n]) * fadeIn * fadeOut;
  let r = (music.R[n] * 0.95 + sfx.R[n] * 0.8 + sv.R[n] + endBus.R[n]) * fadeIn * fadeOut;
  l = hpL.p(l); r = hpR.p(r);
  outL[n] = l; outR[n] = r;
  peak = Math.max(peak, Math.abs(l), Math.abs(r));
}
// мягкое ограничение и нормализация к -1 дБ
const g = 0.89 / peak;
const sat = x => Math.tanh(x * 1.2) / Math.tanh(1.2);
const buf = Buffer.alloc(44 + N * 2 * 4);
buf.write('RIFF', 0); buf.writeUInt32LE(36 + N * 8, 4); buf.write('WAVE', 8);
buf.write('fmt ', 12); buf.writeUInt32LE(16, 16); buf.writeUInt16LE(3, 20); buf.writeUInt16LE(2, 22);
buf.writeUInt32LE(SR, 24); buf.writeUInt32LE(SR * 8, 28); buf.writeUInt16LE(8, 32); buf.writeUInt16LE(32, 34);
buf.write('data', 36); buf.writeUInt32LE(N * 8, 40);
for (let n = 0; n < N; n++) {
  buf.writeFloatLE(sat(outL[n] * g) * 0.95, 44 + n * 8);
  buf.writeFloatLE(sat(outR[n] * g) * 0.95, 48 + n * 8);
}
fs.writeFileSync(process.argv[2] || 'audio.wav', buf);
console.log('peak before norm', peak.toFixed(3), 'samples', N);
