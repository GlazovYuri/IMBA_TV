// Видео о проекте одной командой: npm run video (в tools/brag) или node tools/brag/make.mjs
//   --reuse-site    не снимать сайт заново, взять снимки из прошлого запуска
//   --stills 1,2.5  только кадры в PNG для проверки (brag-output/work/stills)
// Результат: brag-output/brag.mp4, brag.jpg, share-copy.txt. Промежуточное - в brag-output/work
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { buildOled } from './oled.mjs';
import { captureSite } from './site.mjs';
import { renderVideo, renderStills } from './render.mjs';

const HERE = path.dirname(new URL(import.meta.url).pathname);
const ROOT = path.resolve(HERE, '../..');
const OUT = path.join(ROOT, 'brag-output');
const WORK = path.join(OUT, 'work');
const args = process.argv.slice(2);
const flag = name => args.includes(name);
const value = name => args[args.indexOf(name) + 1];
const run = (cmd, a) => execFileSync(cmd, a, { stdio: 'inherit' });
const step = msg => console.log(`\n== ${msg}`);

fs.mkdirSync(WORK, { recursive: true });

step('Кадры экрана из кода прошивки');
const events = buildOled(WORK);

if (!flag('--reuse-site') || !fs.existsSync(path.join(WORK, 'site/site.json'))) {
  step('Материал с сайта');
  await captureSite(WORK);
}

if (flag('--stills')) {
  step('Кадры для проверки');
  await renderStills(WORK, path.join(WORK, 'stills'), value('--stills').split(',').map(Number));
  process.exit(0);
}

step('Видео');
const video = path.join(WORK, 'video.mp4');
const { duration, scenes } = await renderVideo(WORK, video);

step('Музыка и звуки');
const { EDIT, FLASH, CRT, ...S } = scenes;
fs.writeFileSync(path.join(WORK, 'timeline.json'), JSON.stringify({ duration, S, EDIT, FLASH, CRT, blinks: events.alarm.invert_on }, null, 1));
const raw = path.join(WORK, 'audio.wav'), norm = path.join(WORK, 'audio-norm.wav');
run('node', [path.join(HERE, 'synth.mjs'), raw, path.join(WORK, 'timeline.json')]);
// громкость для соцсетей: -14 LUFS, пик -1 дБ, в два прохода
const stats = JSON.parse(execFileSync('sh', ['-c', `ffmpeg -hide_banner -i "${raw}" -af loudnorm=I=-14:TP=-1.0:LRA=11:print_format=json -f null - 2>&1 | sed -n '/^{/,/^}/p'`]).toString());
run('ffmpeg', ['-loglevel', 'error', '-y', '-i', raw, '-af',
  `loudnorm=I=-14:TP=-1.0:LRA=11:measured_I=${stats.input_i}:measured_TP=${stats.input_tp}:measured_LRA=${stats.input_lra}:measured_thresh=${stats.input_thresh}:offset=${stats.target_offset}:linear=true,aresample=48000`,
  '-c:a', 'pcm_f32le', norm]);

step('Постер и сборка');
// постер: титульный лист, всё на месте (рис. 1 и надписи)
const posterT = S.cut + 2.2;
const poster = path.join(OUT, 'brag.jpg');
run('ffmpeg', ['-loglevel', 'error', '-y', '-ss', posterT.toFixed(3), '-i', video, '-frames:v', '1', '-q:v', '2', poster]);
// постер вместо кадра 0, длительность и синхронизация звука не меняются
run('ffmpeg', ['-loglevel', 'error', '-y', '-i', video, '-loop', '1', '-i', poster, '-i', norm,
  '-filter_complex', "[1:v]format=yuv420p[p];[0:v][p]overlay=0:0:enable='eq(n,0)':shortest=1,format=yuv420p[v]",
  '-map', '[v]', '-map', '2:a', '-c:v', 'libx264', '-preset', 'slow', '-crf', '16', '-tune', 'animation', '-pix_fmt', 'yuv420p', '-r', '30',
  '-c:a', 'aac', '-b:a', '192k', '-ar', '48000', '-movflags', '+faststart', '-shortest', path.join(OUT, 'brag.mp4')]);
fs.copyFileSync(path.join(HERE, 'share-copy.txt'), path.join(OUT, 'share-copy.txt'));
console.log(`\nГотово: ${path.join(OUT, 'brag.mp4')} (${duration.toFixed(2)} с)`);
