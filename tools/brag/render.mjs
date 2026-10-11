// Рендер композиции: кадры из headless Chrome в ffmpeg.
//   renderVideo(work, out)            - всё видео без звука
//   renderStills(work, dir, [t, ...]) - отдельные кадры в PNG для проверки
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { serve } from './server.mjs';
import { launch } from './browser.mjs';

async function openComposition(workDir) {
  const { server, url } = await serve(workDir);
  const browser = await launch();
  const page = await browser.newPage();
  page.on('pageerror', e => console.log('[ошибка на странице]', e.message));
  page.on('console', m => { if (m.type() === 'error') console.log('[консоль]', m.text()); });
  await page.setViewport({ width: 1920, height: 1080, deviceScaleFactor: 1 });
  await page.emulateMediaFeatures([{ name: 'prefers-color-scheme', value: 'light' }]);
  await page.goto(url, { waitUntil: 'load' });
  await page.waitForFunction('window.__ready', { timeout: 60000 });
  await page.evaluate(() => window.__ready);
  const close = async () => { await browser.close(); server.close(); };
  return { page, close, duration: await page.evaluate(() => window.DURATION), scenes: await page.evaluate(() => window.SCENES) };
}

export async function renderStills(workDir, dir, times) {
  const { page, close } = await openComposition(workDir);
  fs.mkdirSync(dir, { recursive: true });
  for (const t of times) {
    await page.evaluate(t => window.renderFrame(t), t);
    await page.screenshot({ path: path.join(dir, `t${t.toFixed(2)}.png`) });
  }
  await close();
}

export async function renderVideo(workDir, out, fps = 30) {
  const { page, close, duration, scenes } = await openComposition(workDir);
  const frames = Math.round(duration * fps);
  const ff = spawn('ffmpeg', ['-loglevel', 'error', '-y', '-f', 'image2pipe', '-framerate', String(fps), '-c:v', 'png', '-i', '-',
    '-c:v', 'libx264', '-preset', 'slow', '-crf', '14', '-pix_fmt', 'yuv420p', '-tune', 'animation', '-movflags', '+faststart', out], { stdio: ['pipe', 'inherit', 'inherit'] });
  const cdp = await page.createCDPSession();
  const t0 = Date.now();
  for (let n = 0; n < frames; n++) {
    await page.evaluate(t => window.renderFrame(t), n / fps);
    const { data } = await cdp.send('Page.captureScreenshot', { format: 'png', optimizeForSpeed: true });
    if (!ff.stdin.write(Buffer.from(data, 'base64'))) await new Promise(r => ff.stdin.once('drain', r));
    if (n % 150 === 0) console.log(`кадр ${n}/${frames}, ${((Date.now() - t0) / 1000).toFixed(0)} с`);
  }
  ff.stdin.end();
  await new Promise(r => ff.on('close', r));
  await close();
  return { duration: frames / fps, scenes };
}

if (process.argv[1] === new URL(import.meta.url).pathname) {
  const [work, mode, ...rest] = process.argv.slice(2);
  if (mode === 'stills') await renderStills(path.resolve(work), path.resolve(rest[0]), rest[1].split(',').map(Number));
  else console.log(await renderVideo(path.resolve(work), path.resolve(mode)));
}
