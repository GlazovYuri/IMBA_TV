// Headless Chrome для съёмки сайта и рендера кадров
import fs from 'node:fs';
import puppeteer from 'puppeteer-core';

const CANDIDATES = [
  process.env.CHROME,
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  '/usr/bin/google-chrome',
  '/usr/bin/chromium',
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
];

export function launch(extra = []) {
  const executablePath = CANDIDATES.find(p => p && fs.existsSync(p));
  if (!executablePath) throw new Error('Не найден Chrome, задайте CHROME=путь/к/chrome');
  return puppeteer.launch({
    executablePath,
    headless: true,
    args: ['--hide-scrollbars', '--force-color-profile=srgb', '--font-render-hinting=none', '--ignore-gpu-blocklist', ...extra],
  });
}
