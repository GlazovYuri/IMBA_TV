# Видео о проекте

Ролик на 40 секунд: корпус IMBA TV в 3D влетает и включается, камера уходит в экран, дальше страницы в стиле сайта показывают подключение к LoEUC, открытые исходники, основной экран и тревогу по ШИМ, редактор экранов, анимации и прошивку по USB-C и Bluetooth.

Кадры экрана рисует настоящий код прошивки: [harness.cpp](harness.cpp) собирает экраны и анимации из `IMBA_TV/` вместе с GyverOLED и эмулятором SSH1106 из [tools/previews](../previews/). Редактор экранов и блок прошивки сняты с живого сайта. Музыку и звуки синтезирует [synth.mjs](synth.mjs).

## Сборка

Нужны Node.js 22+, ffmpeg, компилятор C++, Google Chrome и библиотека GyverOLED (та же, что для Arduino IDE).

```bash
cd tools/brag
npm install
npm run video
```

Результат в `brag-output/` в корне репозитория: `brag.mp4`, постер `brag.jpg` и текст для поста `share-copy.txt`. Промежуточные файлы лежат в `brag-output/work/`. Папка в git не попадает.

* `--reuse-site` не снимать сайт заново
* `--stills 1,2.5,10` только кадры в PNG для проверки (`brag-output/work/stills/`)

Пути к Chrome и GyverOLED можно задать переменными `CHROME` и `GYVEROLED`.

## Что где

* [make.mjs](make.mjs): все шаги по порядку
* [harness.cpp](harness.cpp), [oled.mjs](oled.mjs): сцены экрана из кода прошивки
* [site.mjs](site.mjs): снимки и разметка с сайта
* [composition/](composition/): сам ролик, каждый кадр рисуется как функция времени; [device.js](composition/device.js) - 3D-модель корпуса
* [render.mjs](render.mjs): кадры из headless Chrome в ffmpeg
* [synth.mjs](synth.mjs): музыка и звуки, 125 BPM, ля минор
* [plan.md](plan.md): сценарий по сценам
