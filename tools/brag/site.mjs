// Материал с живого сайта (светлая тема): разметка операции 3, состояния редактора экранов
// (снимки с двойной плотностью и координаты нажатий), размер прошивки последней версии.
// Пишет в <out>/site/
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { launch } from './browser.mjs';

const SITE = 'https://glazovyuri.github.io/IMBA_TV/';
const sleep = ms => new Promise(r => setTimeout(r, ms));

export async function captureSite(outDir) {
  const dir = path.join(outDir, 'site');
  fs.mkdirSync(dir, { recursive: true });
  const browser = await launch();
  const page = await browser.newPage();
  await page.emulateMediaFeatures([{ name: 'prefers-color-scheme', value: 'light' }, { name: 'prefers-reduced-motion', value: 'reduce' }]);

  // ---------- редактор экранов: окно 1100×900, группа «Экраны» вверху
  await page.setViewport({ width: 1100, height: 900, deviceScaleFactor: 2 });
  await page.goto(SITE, { waitUntil: 'networkidle0' });
  await page.waitForSelector('#config-form .group .screen-zone, #config-form .group canvas', { timeout: 30000 }).catch(() => {});
  await page.waitForSelector('#config-form .group', { timeout: 30000 });
  const info = { version: await page.$eval('#runhead-rev', e => e.textContent.trim()) };

  await page.evaluate(() => {
    const g = document.querySelector('#config-form .group');
    g.open = true;
    document.querySelector('.runhead')?.style.setProperty('position', 'static');
  });
  await sleep(400);
  const scrollTo = async () => page.evaluate(() => {
    const g = document.querySelector('#config-form .group');
    window.scrollTo(0, g.getBoundingClientRect().top + window.scrollY - 24);
  });
  await scrollTo();
  await sleep(300);
  const box = async sel => page.evaluate(sel => {
    const e = typeof sel === 'string' ? document.querySelector(sel) : null;
    if (!e) return null;
    const b = e.getBoundingClientRect();
    return { x: b.left, y: b.top, w: b.width, h: b.height };
  }, sel);
  const shot = async name => { await page.screenshot({ path: path.join(dir, `editor-${name}.png`) }); };

  await shot('list');
  // первый экран: шестерёнка
  info.gear = await page.evaluate(() => {
    const g = document.querySelector('#config-form .group');
    const btn = g.querySelector('.tool-gear');
    btn.id = 'v-gear';
    const b = btn.getBoundingClientRect();
    return { x: b.left, y: b.top, w: b.width, h: b.height };
  });
  await page.click('#v-gear');
  await sleep(500);
  await shot('dialog');
  // правое место на экране (ШИМ)
  info.zone = await page.evaluate(() => {
    const zones = [...document.querySelectorAll('dialog[open] .screen-zone')];
    const z = zones.find(z => /ШИМ/.test(z.getAttribute('aria-label') || '')) || zones[zones.length - 1];
    z.id = 'v-zone';
    const b = z.getBoundingClientRect();
    return { x: b.left, y: b.top, w: b.width, h: b.height, label: z.getAttribute('aria-label') };
  });
  await page.click('#v-zone');
  await sleep(400);
  await shot('zone');
  // значение «Температура»
  info.option = await page.evaluate(() => {
    const leaf = [...document.querySelectorAll('dialog[open] *')].find(e => !e.children.length && /^Температура$/.test(e.textContent.trim()));
    const btn = leaf.closest('button, label') || leaf;
    btn.id = 'v-temp';
    const b = btn.getBoundingClientRect();
    return { x: b.left, y: b.top, w: b.width, h: b.height, text: btn.textContent.trim() };
  });
  await page.click('#v-temp');
  await sleep(400);
  await shot('temp');
  info.done = await page.evaluate(() => {
    const leaf = [...document.querySelectorAll('dialog[open] *')].find(e => !e.children.length && /^Готово$/.test(e.textContent.trim()));
    const btn = leaf.closest('button, label') || leaf;
    btn.id = 'v-done';
    const b = btn.getBoundingClientRect();
    return { x: b.left, y: b.top, w: b.width, h: b.height };
  });
  await page.click('#v-done');
  await sleep(500);
  await shot('done');

  // ---------- операция 3: разметка, вкладки USB и Bluetooth
  await page.setViewport({ width: 1280, height: 900, deviceScaleFactor: 1 });
  await page.goto(SITE, { waitUntil: 'networkidle0' });
  await page.waitForSelector('#config-form .group', { timeout: 30000 });
  await sleep(300);
  fs.writeFileSync(path.join(dir, 'op3-usb.html'), await page.$eval('#op3', e => e.outerHTML));
  await page.click('#tab-ble');
  await sleep(300);
  fs.writeFileSync(path.join(dir, 'op3-ble.html'), await page.$eval('#op3', e => e.outerHTML));
  const sprite = fs.readFileSync(path.join(path.dirname(new URL(import.meta.url).pathname), '../../web/index.html'), 'utf8')
    .match(/<svg class="sprite"[\s\S]*?<\/svg>/)[0];
  fs.writeFileSync(path.join(dir, 'sprite.svg'), sprite);
  await browser.close();

  // размер прошивки выбранной по умолчанию версии
  const index = await (await fetch(SITE + 'firmware/index.json')).json();
  const stable = index.versions.find(v => !v.prerelease);
  const zip = Buffer.from(await (await fetch(SITE + stable.files.zip)).arrayBuffer());
  fs.writeFileSync(path.join(dir, 'firmware.zip'), zip);
  const list = execFileSync('unzip', ['-l', path.join(dir, 'firmware.zip')]).toString();
  info.firmware = { tag: stable.tag, bin: Number(list.match(/^\s*(\d+)\s+\S+\s+\S+\s+\S+\.bin$/m)[1]) };
  fs.writeFileSync(path.join(dir, 'site.json'), JSON.stringify(info, null, 1));
  return info;
}

if (process.argv[1] === new URL(import.meta.url).pathname) {
  console.log(await captureSite(path.resolve(process.argv[2] || 'brag-output/work')));
}
