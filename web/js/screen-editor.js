// Редактор экранов: до пяти экранов дисплея в том порядке, в каком они переключаются кнопкой.
// Экраны лежат в настройках слотами screenN_type и screenN_v1..v4 (IMBA_TV/config.json, группа с widget: "screens"):
// сначала экраны по порядку, затем пустые слоты. Каждый экран показан так, как его нарисует прошивка (screens.js),
// у настраиваемых шестерёнка открывает экран крупно: место на нём выбирается нажатием, значение - кнопками под ним.

import { renderScreen, SCREEN, SCREEN_TYPES, VALUE_NAMES, normalizeScreen } from './screens.js';
import { paint } from './oled.js';

const SLOT_KEY = /^screen(\d+)_(type|v([1-4]))$/;

export const isScreenField = (f) => SLOT_KEY.test(f.key);

// Слоты экранов из описания настроек: [{ type: поле, v: [4 поля] }] или null, если их нет
export function screenSlots(fields) {
  const slots = [];
  for (const f of fields) {
    const m = SLOT_KEY.exec(f.key);
    if (!m) continue;
    const n = Number(m[1]) - 1;
    slots[n] ??= { type: null, v: [] };
    if (m[2] === 'type') slots[n].type = f;
    else slots[n].v[Number(m[3]) - 1] = f;
  }
  const complete = slots.length > 0 && [...slots].every((s) => s?.type && [0, 1, 2, 3].every((k) => s.v[k]));
  return complete ? slots : null;
}

// Экраны по порядку, пустые слоты пропускаются - как их читает прошивка
export function readScreens(slots, values) {
  return slots
    .map((s) => ({ type: values[s.type.key], v: s.v.map((f) => values[f.key]) }))
    .filter((s) => s.type !== SCREEN.NONE);
}

// Записывает экраны в слоты по порядку, остальные слоты пустые со значениями по умолчанию
export function writeScreens(slots, values, screens) {
  slots.forEach((s, i) => {
    const screen = screens[i];
    values[s.type.key] = screen ? screen.type : SCREEN.NONE;
    s.v.forEach((f, k) => {
      values[f.key] = screen ? screen.v[k] : f.default;
    });
  });
}

// Какие экраны можно добавить: те, что знают и сайт, и выбранная прошивка (варианты screenN_type в её настройках).
// Старая прошивка заменила бы незнакомый тип экраном по умолчанию
const ADDABLE = [SCREEN.MAIN, SCREEN.GRID, SCREEN.INFO, SCREEN.CLOCK];
export function addableTypes(slots) {
  const options = slots[0]?.type.options;
  return options ? ADDABLE.filter((t) => options.some((o) => o.value === t)) : ADDABLE;
}

// Какие значения можно выбрать на месте: те, что знает выбранная прошивка (варианты screenN_vK в её настройках,
// у всех полей они одинаковые). Старая прошивка заменила бы незнакомое значение значением по умолчанию
export function selectableValues(slots, values) {
  const options = slots[0]?.v[0]?.options;
  return options ? values.filter((v) => options.some((o) => o.value === v)) : values;
}

export function newScreen(type) {
  return { type, v: [...(SCREEN_TYPES[type]?.defaults ?? [0, 0, 0, 0])] };
}

export function moveScreen(screens, from, to) {
  const next = [...screens];
  const [item] = next.splice(from, 1);
  next.splice(to, 0, item);
  return next;
}

// Значения экрана, которые на нём видны: у пустого и у экрана связи их нет
function visible(screen) {
  const s = normalizeScreen(screen);
  return { type: s.type, v: s.v.slice(0, SCREEN_TYPES[s.type]?.slots.length ?? 0) };
}

// Сколько мест в списке экранов отличается от настроек прошивки по умолчанию
export function changedScreens(slots, values) {
  const now = readScreens(slots, values).map(visible);
  const base = readScreens(slots, Object.fromEntries(slots.flatMap((s) => [s.type, ...s.v]).map((f) => [f.key, f.default]))).map(visible);
  let changed = 0;
  for (let i = 0; i < Math.max(now.length, base.length); i++) {
    if (JSON.stringify(now[i]) !== JSON.stringify(base[i])) changed++;
  }
  return changed;
}

// аббревиатуры вроде «ШИМ» остаются заглавными
const lower = (text) => (/^\p{Lu}{2}/u.test(text) ? text : text.charAt(0).toLowerCase() + text.slice(1));
const upper = (text) => text.charAt(0).toUpperCase() + text.slice(1);

// Что на экране, одной строкой
export function describeScreen(screen) {
  const s = normalizeScreen(screen);
  const name = (v) => lower(VALUE_NAMES[v] ?? '?');
  if (s.type === SCREEN.MAIN) return `Сверху ${name(s.v[0])}, крупно ${name(s.v[1])} и ${name(s.v[2])}`;
  if (s.type === SCREEN.GRID) return upper(s.v.map(name).join(', '));
  return SCREEN_TYPES[s.type]?.about ?? '';
}

export function describeScreens(slots, values) {
  return readScreens(slots, values)
    .map((s, i) => `${i + 1}) ${SCREEN_TYPES[s.type]?.name ?? 'экран'}: ${lower(describeScreen(s))}`)
    .join('; ');
}

// Рамка места на экране: зажжённые им точки с отступом, в пределах экрана; без точек - место целиком
const ZONE_PAD = 2;
export function zoneBox(box, [x, y, w, h]) {
  if (!box) return [x, y, x + w - 1, y + h - 1];
  return [
    Math.max(box[0] - ZONE_PAD, 0),
    Math.max(box[1] - ZONE_PAD, 0),
    Math.min(box[2] + ZONE_PAD, 127),
    Math.min(box[3] + ZONE_PAD, 63),
  ];
}

// ---------- Разметка ----------

function el(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text != null) node.textContent = text;
  return node;
}

function iconButton(icon, label, className = '') {
  const button = el('button', `tool ${className}`.trim());
  button.type = 'button';
  button.title = label;
  button.setAttribute('aria-label', label);
  button.innerHTML = `<svg class="icon" aria-hidden="true"><use href="#${icon}"/></svg>`;
  return button;
}

function canvas(className) {
  const c = el('canvas', className);
  c.width = 128;
  c.height = 64;
  return c;
}

// options(): { wheelVoltage, firmware, bootloader } - для отрисовки; onChange() - после каждого изменения;
// isBusy() - идёт прошивка, менять нельзя
export function createScreenEditor({ slots, values, options, onChange, isBusy }) {
  const root = el('div', 'screens');
  const list = el('ol', 'screen-list');
  list.setAttribute('aria-label', 'Экраны по порядку переключения');

  const add = el('div', 'screen-add');
  const addTitle = el('p', 'screen-add-title', 'Добавить экран');
  const tiles = el('div', 'screen-tiles');
  const full = el('p', 'screen-add-full', '');
  add.append(addTitle, tiles, full);
  root.append(list, add);

  const draw = (target, screen) => paint(target, renderScreen(screen, options()));
  const screens = () => readScreens(slots, values());

  function commit(next, focus) {
    writeScreens(slots, values(), next);
    onChange();
    render();
    focus?.();
  }

  // ---------- окно настройки экрана ----------

  const dialog = el('dialog', 'screen-dialog');
  const head = el('div', 'screen-dialog-head');
  const title = el('h3', 'screen-dialog-title');
  title.id = 'screen-dialog-title';
  dialog.setAttribute('aria-labelledby', title.id);
  const close = iconButton('i-x', 'Закрыть', 'tool-plain');
  head.append(title, close);
  const stage = el('div', 'screen-stage');
  const big = canvas('screen-stage-canvas');
  const zones = el('div', 'screen-zones');
  stage.append(big, zones);
  const hint = el('p', 'screen-hint', 'Нажмите на место на экране и выберите, что в нём показывать.');
  const choice = el('fieldset', 'screen-choice');
  const foot = el('div', 'screen-dialog-foot');
  const done = el('button', 'button button-primary', 'Готово');
  done.type = 'button';
  foot.append(done);
  dialog.append(head, stage, hint, choice, foot);
  root.append(dialog);

  let editing = -1;  // номер экрана в окне
  let slot = 0;  // выбранное место на нём
  // экран в окне увеличен в целое число раз: точки дисплея и рамки мест ложатся на сетку пикселей
  let scale = 4;

  function fitScale() {
    // экран стоит по центру, поля окна - как у подсказки под ним
    const style = getComputedStyle(stage);
    const gutter = parseFloat(getComputedStyle(hint).marginLeft);
    const frame = 2 * gutter + parseFloat(style.paddingLeft) + parseFloat(style.paddingRight) +
      parseFloat(style.borderLeftWidth) + parseFloat(style.borderRightWidth);
    return Math.max(1, Math.min(4, Math.floor((dialog.clientWidth - frame) / 128)));
  }
  new ResizeObserver(() => {
    if (dialog.open && fitScale() !== scale) renderEditor();
  }).observe(dialog);

  close.addEventListener('click', () => dialog.close());
  done.addEventListener('click', () => dialog.close());
  // нажатие на затемнение вокруг окна закрывает его (поля внутри окна - тоже сам dialog, их не считаем)
  dialog.addEventListener('click', (e) => {
    const r = dialog.getBoundingClientRect();
    const outside = e.clientX < r.left || e.clientX > r.right || e.clientY < r.top || e.clientY > r.bottom;
    if (e.target === dialog && outside) dialog.close();
  });

  function openEditor(index) {
    editing = index;
    slot = 0;
    dialog.showModal();
    // масштаб считается по ширине открытого окна
    renderEditor();
    zones.querySelector('[aria-pressed="true"]')?.focus();
  }

  function renderEditor() {
    const screen = screens()[editing];
    const info = screen && SCREEN_TYPES[screen.type];
    if (!info?.slots.length) {
      if (dialog.open) dialog.close();
      return;
    }
    title.textContent = `Экран ${editing + 1} · ${info.name}`;
    const oled = renderScreen(screen, options());
    paint(big, oled);
    scale = dialog.open ? fitScale() : scale;
    big.style.width = `${128 * scale}px`;
    big.style.height = `${64 * scale}px`;
    const shown = normalizeScreen(screen).v;

    zones.replaceChildren(...info.slots.map((s, i) => {
      // рамка по точкам, которые рисует это место, с отступом в 2 точки дисплея
      const [x0, y0, x1, y1] = zoneBox(oled.boxes.get(i), s.zone);
      const zone = el('button', 'screen-zone');
      zone.type = 'button';
      zone.style.left = `${x0 * scale}px`;
      zone.style.top = `${y0 * scale}px`;
      zone.style.width = `${(x1 - x0 + 1) * scale}px`;
      zone.style.height = `${(y1 - y0 + 1) * scale}px`;
      zone.setAttribute('aria-pressed', String(i === slot));
      zone.setAttribute('aria-label', `${s.label}: ${VALUE_NAMES[shown[i]]}`);
      zone.title = s.label;
      zone.addEventListener('click', () => {
        slot = i;
        renderEditor();
        zones.children[i]?.focus();
      });
      return zone;
    }));

    const current = info.slots[slot];
    const legend = el('legend', '', current.label);
    const chips = el('div', 'chips');
    for (const value of selectableValues(slots, current.values)) {
      const label = el('label', 'chip');
      const input = el('input');
      input.type = 'radio';
      input.name = 'screen-value';
      input.value = value;
      input.checked = shown[slot] === value;
      input.disabled = isBusy();
      input.addEventListener('change', () => {
        const next = screens();
        next[editing].v[slot] = value;
        commit(next, () => choice.querySelector('input:checked')?.focus());
      });
      label.append(input, el('span', '', VALUE_NAMES[value]));
      chips.append(label);
    }
    choice.replaceChildren(legend, chips);
  }

  // ---------- список ----------

  function render() {
    const all = screens();
    const busy = isBusy();
    const max = slots.length;

    list.replaceChildren(...all.map((screen, i) => {
      const info = SCREEN_TYPES[screen.type];
      const configurable = !!info?.slots.length;
      const item = el('li', 'screen-card');
      const num = el('span', 'screen-num', String(i + 1));
      num.setAttribute('aria-hidden', 'true');

      const shot = el(configurable ? 'button' : 'div', 'screen-shot');
      const picture = canvas();
      shot.append(picture);
      draw(picture, screen);
      if (configurable) {
        shot.type = 'button';
        shot.disabled = busy;
        shot.setAttribute('aria-label', `Настроить экран ${i + 1}`);
        shot.addEventListener('click', () => openEditor(i));
      }

      const text = el('div', 'screen-text');
      text.append(el('span', 'screen-name', `Экран ${i + 1}. ${info?.name ?? 'Неизвестный'}`), el('span', 'screen-desc', describeScreen(screen)));

      const tools = el('div', 'screen-tools');
      if (configurable) {
        const gear = iconButton('i-settings', `Настроить экран ${i + 1}`, 'tool-gear');
        gear.disabled = busy;
        gear.addEventListener('click', () => openEditor(i));
        tools.append(gear);
      }
      const up = iconButton('i-arrow-up', `Экран ${i + 1} раньше`);
      up.disabled = busy || i === 0;
      up.addEventListener('click', () => commit(moveScreen(all, i, i - 1), () => focusTool(i - 1, 'up', 'down')));
      const down = iconButton('i-arrow-down', `Экран ${i + 1} позже`);
      down.disabled = busy || i === all.length - 1;
      down.addEventListener('click', () => commit(moveScreen(all, i, i + 1), () => focusTool(i + 1, 'down', 'up')));
      const remove = iconButton('i-trash', `Удалить экран ${i + 1}`, 'tool-danger');
      remove.disabled = busy || all.length < 2;
      remove.title = all.length < 2 ? 'Нужен хотя бы один экран' : remove.title;
      remove.addEventListener('click', () => commit(all.filter((_, k) => k !== i), () => tiles.querySelector('button:not(:disabled)')?.focus()));
      up.dataset.tool = 'up';
      down.dataset.tool = 'down';
      tools.append(up, down, remove);

      item.append(num, shot, text, tools);
      return item;
    }));

    const canAdd = all.length < max && !busy;
    tiles.replaceChildren(...addableTypes(slots).map((type) => {
      const tile = el('button', 'screen-tile');
      tile.type = 'button';
      tile.disabled = !canAdd;
      const picture = canvas();
      draw(picture, newScreen(type));
      const name = el('span', 'screen-tile-name');
      name.innerHTML = '<svg class="icon" aria-hidden="true"><use href="#i-plus"/></svg>';
      name.append(SCREEN_TYPES[type].name);
      tile.setAttribute('aria-label', `Добавить экран: ${SCREEN_TYPES[type].name}`);
      tile.append(picture, name);
      tile.addEventListener('click', () => {
        const next = [...screens(), newScreen(type)];
        commit(next, () => list.lastElementChild?.querySelector('.tool-gear, .tool')?.focus());
      });
      return tile;
    }));
    full.textContent = all.length >= max
      ? `Экранов уже ${max}, больше не поместится. Удалите какой-нибудь, чтобы добавить новый.`
      : `Можно добавить ещё ${max - all.length} из ${max}. Экраны переключаются коротким нажатием кнопки сверху вниз.`;

    if (dialog.open) renderEditor();
  }

  // после перестановки фокус остаётся на той же стрелке у переехавшего экрана, а если она погасла - на соседней
  function focusTool(index, tool, other) {
    const card = list.children[index];
    const button = card?.querySelector(`[data-tool="${tool}"]`);
    (button && !button.disabled ? button : card?.querySelector(`[data-tool="${other}"]`))?.focus();
  }

  render();
  return { element: root, refresh: render };
}
