// Дисплей 128x64 для предпросмотра экранов: то, что делает с буфером библиотека GyverOLED
// (режим с буфером), до особенностей вроде сдвига текста по y и линии, не доходящей до края.
// Код экранов прошивки (web/js/screens.js) рисует через эти же вызовы, и картинка совпадает с дисплеем:
// это проверяет web/test/screens.test.mjs по кадрам от настоящей прошивки.

import { FONT_HEX } from './oled-font.js';

export const WIDTH = 128;
export const HEIGHT = 64;
export const OLED_CLEAR = 0;
export const OLED_FILL = 1;
export const OLED_STROKE = 2;
export const BUF_ADD = 0;
export const BUF_SUBTRACT = 1;
export const BUF_REPLACE = 2;

const MAX_X = WIDTH - 1;
const MAX_Y = HEIGHT - 1;
const MAX_ROW = 7;
const FONT = Uint8Array.from(FONT_HEX.match(/../g), (h) => parseInt(h, 16));
const utf8 = new TextEncoder();

export class Oled {
  constructor() {
    // как в GyverOLED: столбец за столбцом, по 8 байт (страниц) на столбец, младший бит сверху
    this.buffer = new Uint8Array(WIDTH * HEIGHT / 8);
    this.x = 0;
    this.y = 0;
    this.scaleX = 1;
    this.scaleY = 8;
    this.shift = 0;
    this.mode = BUF_REPLACE;
    this.lastChar = 0;
  }

  // ---------- буфер ----------

  index(x, y) {
    return (y >> 3) + (x << 3);
  }

  // как writeData: байт в столбец курсора со сдвигом offsetX и на offsetY страниц ниже, без проверки границ.
  // В прошивке выход за буфер портит память, здесь такая запись пропускается
  writeData(data, offsetY = 0, offsetX = 0, mode = BUF_ADD) {
    const i = this.index(this.x + offsetX, this.y) + offsetY;
    if (i < 0 || i >= this.buffer.length) return;
    data &= 0xff;
    if (mode === BUF_ADD) this.buffer[i] |= data;
    else if (mode === BUF_SUBTRACT) this.buffer[i] &= ~data;
    else this.buffer[i] = data;
  }

  clear() {
    this.buffer.fill(0);
    this.setCursorXY(this.x, this.y);
  }

  // память дисплея страница за страницей (как её получает контроллер)
  pages() {
    const out = new Uint8Array(this.buffer.length);
    for (let x = 0; x < WIDTH; x++) {
      for (let page = 0; page < 8; page++) out[page * WIDTH + x] = this.buffer[(x << 3) + page];
    }
    return out;
  }

  pixel(x, y) {
    return (this.buffer[this.index(x, y)] >> (y & 7)) & 1;
  }

  // ---------- текст ----------

  setCursorXY(x, y) {
    this.x = x;
    this.y = y;
    this.shift = y & 7;
  }

  setCursor(x, row) {
    this.setCursorXY(x, row << 3);
  }

  setScale(scale) {
    scale = Math.min(Math.max(scale, 1), 4);
    this.scaleX = scale;
    this.scaleY = scale * 8;
    this.setCursorXY(this.x, this.y);
  }

  textMode(mode) {
    this.mode = mode;
  }

  // число печатается как в Arduino Print, строка - байтами UTF-8
  print(value) {
    for (const byte of utf8.encode(String(value))) this.write(byte);
  }

  glyph(code, col) {
    if (col > 4) return 0;
    let font = code - 32;
    if (font <= 95) {
      // как есть
    } else if (font <= 111) font += 47;
    else if (font <= 159) font -= 17;
    else font -= 1;
    return FONT[font * 5 + col] ?? 0;
  }

  write(data) {
    if (this.y + this.scaleY > MAX_Y + 1) data = 0;
    // кириллица: первый байт UTF-8 пропускается, второй выбирает букву
    if (data > 127) {
      const thisData = data;
      if (data > 191) data = 0;
      else if (this.lastChar === 209 && data === 145) data = 192;
      else if (this.lastChar === 208 && data === 129) data = 149;
      else if (this.lastChar === 226 && data === 128) data = 0;
      else if (this.lastChar === 128 && data === 148) data = 45;
      this.lastChar = thisData;
    }
    if (data === 0) return;

    const newX = this.x + this.scaleX * 6;
    if (newX < 0 || this.x > MAX_X) {
      this.x = newX;
      return;
    }
    const sx = this.scaleX;
    for (let col = 0; col < 6; col++) {
      const bits = this.glyph(data, col);
      if (sx === 1) {
        if (this.x >= 0 && this.x <= MAX_X) {
          if (this.shift === 0) this.writeData(bits, 0, 0, this.mode);
          else {
            this.writeData(bits << this.shift, 0, 0, this.mode);
            this.writeData(bits >> (8 - this.shift), 1, 0, this.mode);
          }
        }
      } else {
        // каждый бит растягивается в sx бит по вертикали
        let stretched = 0n;
        for (let i = 0, count = 0; i < 8; i++) {
          for (let j = 0; j < sx; j++, count++) if ((bits >> i) & 1) stretched |= 1n << BigInt(count);
        }
        for (let i = 0; i < sx; i++) {
          let prev = 0;
          if (this.x + i >= 0 && this.x + i <= MAX_X) {
            for (let j = 0; j < sx; j++) {
              const part = Number((stretched >> BigInt(j * 8)) & 0xffn);
              if (this.shift === 0) this.writeData(part, j, i, this.mode);
              else {
                this.writeData((prev >> (8 - this.shift)) | (part << this.shift), j, i, this.mode);
                prev = part;
              }
            }
          }
          if (this.shift !== 0) this.writeData(prev >> (8 - this.shift), sx, i, this.mode);
        }
      }
      this.x += sx;
    }
  }

  // байт столбиком: курсор сначала сдвигается на столбец
  drawByte(data) {
    if (++this.x > MAX_X) return;
    if (this.shift === 0) this.writeData(data);
    else {
      this.writeData(data << this.shift);
      this.writeData(data >> (8 - this.shift), 1);
    }
  }

  // ---------- графика ----------

  dot(x, y, fill = 1) {
    if (x < 0 || x > MAX_X || y < 0 || y > MAX_Y) return;
    this.x = 0;
    this.y = 0;
    const i = this.index(x, y);
    if (fill) this.buffer[i] |= 1 << (y & 7);
    else this.buffer[i] &= ~(1 << (y & 7));
  }

  // правый край не включается, если линия доходит до края экрана: так в GyverOLED
  fastLineH(y, x0, x1, fill = 1) {
    this.x = 0;
    this.y = 0;
    if (x0 > x1) [x0, x1] = [x1, x0];
    if (y < 0 || y > MAX_Y) return;
    if (x0 === x1) {
      this.dot(x0, y, fill);
      return;
    }
    x1++;
    x0 = Math.min(Math.max(x0, 0), MAX_X);
    x1 = Math.min(Math.max(x1, 0), MAX_X);
    for (let x = x0; x < x1; x++) this.dot(x, y, fill);
  }

  fastLineV(x, y0, y1, fill = 1) {
    this.x = 0;
    this.y = 0;
    if (y0 > y1) [y0, y1] = [y1, y0];
    if (x < 0 || x > MAX_X) return;
    if (y0 === y1) {
      this.dot(x, y0, fill);
      return;
    }
    for (let y = y0; y < y1 + 1; y++) this.dot(x, y, fill);
  }

  rect(x0, y0, x1, y1, fill = OLED_FILL) {
    this.x = 0;
    this.y = 0;
    if (x0 > x1) [x0, x1] = [x1, x0];
    if (y0 > y1) [y0, y1] = [y1, y0];
    if (fill === OLED_STROKE) {
      this.fastLineH(y0, x0 + 1, x1 - 1);
      this.fastLineH(y1, x0 + 1, x1 - 1);
      this.fastLineV(x0, y0, y1);
      this.fastLineV(x1, y0, y1);
      return;
    }
    if (x0 === x1 && y0 === y1) {
      this.dot(x0, y0, fill);
      return;
    }
    if (x0 === x1) {
      this.fastLineV(x0, y0, y1, fill);
      return;
    }
    if (y0 === y1) {
      this.fastLineH(y0, x0, x1, fill);
      return;
    }
    // заливка байтами по страницам, как в библиотеке: OLED_FILL добавляет, OLED_CLEAR стирает
    const mode = fill === OLED_FILL ? BUF_ADD : BUF_SUBTRACT;
    y1++;
    const shift = y0 & 7;
    let shift2 = 8 - (y1 & 7);
    if (shift2 === 8) shift2 = 0;
    const height = y1 - y0;
    const row0 = y0 >> 3;
    const row1 = (y1 - 1) >> 3;
    const numBytes = row1 - row0;
    x0 = Math.min(Math.max(x0, 0), MAX_X);
    x1 = Math.min(Math.max(x1, 0), MAX_X);
    const put = (data, row, x) => {
      const i = (x << 3) + row;
      if (mode === BUF_ADD) this.buffer[i] |= data & 0xff;
      else this.buffer[i] &= ~data & 0xff;
    };
    for (let x = x0; x <= x1; x++) {
      let row = row0;
      if (numBytes === 0) {
        if (row >= 0 && row <= MAX_ROW) put((255 >> (8 - height)) << shift, row, x);
      } else {
        if (row >= 0 && row <= MAX_ROW) put(255 << shift, row, x);
        row++;
        for (let i = 0; i < numBytes - 1; i++, row++) if (row >= 0 && row <= MAX_ROW) put(255, row, x);
        if (row >= 0 && row <= MAX_ROW) put(255 >> shift2, row, x);
      }
    }
  }

  // битмап столбцами по байту, высота кратна 8
  drawBitmap(x, y, frame, width, height, mode = BUF_ADD) {
    this.x = 0;
    this.y = 0;
    const left = height & 7;
    if (left !== 0) height += 8 - left;
    const shiftY = (y >> 3) + (height >> 3);
    // сдвиг остаётся и для следующего текста, если курсор не переставить
    const shift = this.shift = y & 7;
    const bottom = shift !== 0 && shiftY >= 0 && shiftY <= MAX_ROW;
    const put = (data, row, col) => {
      const i = (col << 3) + row;
      if (i < 0 || i >= this.buffer.length) return;
      if (mode === BUF_ADD) this.buffer[i] |= data & 0xff;
      else if (mode === BUF_SUBTRACT) this.buffer[i] &= ~data & 0xff;
      else this.buffer[i] = data & 0xff;
    };
    for (let X = x, cx = 0; X < x + width; X++, cx++) {
      let prev = 0;
      if (X < 0 || X > MAX_X) continue;
      for (let Y = y >> 3, cy = 0; Y < shiftY; Y++, cy++) {
        const data = frame[cy * width + cx] ?? 0;
        if (shift === 0) {
          if (Y >= 0 && Y <= MAX_ROW) put(data, Y, X);
        } else {
          if (Y >= 0 && Y <= MAX_ROW) put((prev >> (8 - shift)) | (data << shift), Y, X);
          prev = data;
        }
      }
      if (bottom) put(prev >> (8 - shift), shiftY, X);
    }
  }
}

// Кадр на canvas: 1 точка дисплея - 1 пиксель холста, увеличение - через CSS
export function paint(canvas, oled, on = '#eef0f2', off = '#101010') {
  canvas.width = WIDTH;
  canvas.height = HEIGHT;
  const ctx = canvas.getContext('2d');
  const image = ctx.createImageData(WIDTH, HEIGHT);
  const parse = (hex) => [1, 3, 5].map((i) => parseInt(hex.slice(i, i + 2), 16));
  const [onR, onG, onB] = parse(on);
  const [offR, offG, offB] = parse(off);
  for (let y = 0; y < HEIGHT; y++) {
    for (let x = 0; x < WIDTH; x++) {
      const lit = oled.pixel(x, y);
      const i = (y * WIDTH + x) * 4;
      image.data[i] = lit ? onR : offR;
      image.data[i + 1] = lit ? onG : offG;
      image.data[i + 2] = lit ? onB : offB;
      image.data[i + 3] = 255;
    }
  }
  ctx.putImageData(image, 0, 0);
}
