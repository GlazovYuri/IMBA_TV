import { loadFirmware, FirmwareError, withImageCrc, makeUf2 } from './firmware.js';
import { SerialDfu, DfuError, touch1200, activateWaitTime } from './dfu.js';
import { findConfig, readValues, defaultValues, checkValue, isVisible, applyValues, mergeValues } from './config.js';

// USB VID Adafruit: и прошивка, и загрузчик платы (см. boards/boards.txt).
// PID приложения — 0x80xx, загрузчика — 0x00xx.
const ADAFRUIT_VID = 0x239a;
const VISIBLE_VERSIONS = 5;
const SETTINGS_KEY = 'imba-tv-settings';

const $ = (id) => document.getElementById(id);
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

const state = {
  versions: [],
  showAll: false,
  selected: null, // { kind: 'release', version } | { kind: 'own', file }
  firmware: null, // Promise<{ bin, dat, config }>
  firmwareFailed: false,
  fw: null, // загруженная прошивка выбранного варианта
  values: null, // настройки для записи в прошивку
  invalid: new Set(), // поля с ошибкой ввода
  token: 0,
  busy: false,
  cancel: null,
};

const hasSerial = 'serial' in navigator;

// ---------- Журнал и статус ----------

function log(message) {
  const time = new Date().toLocaleTimeString('ru-RU');
  $('log').textContent += `[${time}] ${message}\n`;
  $('log').scrollTop = $('log').scrollHeight;
}

function setStatus(text, kind = '') {
  const el = $('status');
  el.hidden = !text;
  el.textContent = text;
  el.className = `status ${kind}`;
}

function setProgress(done, total) {
  const box = $('progress-box');
  if (total == null) { box.hidden = true; return; }
  const pct = total ? Math.floor((done / total) * 100) : 0;
  box.hidden = false;
  $('progress').value = pct;
  $('progress-text').textContent = `${pct}%`;
}

function formatSize(bytes) {
  return `${(bytes / 1024).toFixed(1).replace('.', ',')} КБ`;
}

function formatDate(iso) {
  const d = new Date(iso);
  return Number.isNaN(d.getTime()) ? '' : d.toLocaleDateString('ru-RU', { day: 'numeric', month: 'long', year: 'numeric' });
}

// ---------- Выбор прошивки ----------

function renderVersions() {
  const box = $('versions');
  box.replaceChildren();

  if (!state.versions.length) {
    const p = document.createElement('p');
    p.className = 'muted';
    p.textContent = 'Опубликованных версий пока нет — можно прошить свой файл.';
    box.append(p);
    $('show-all').hidden = true;
    return;
  }

  const list = state.showAll ? state.versions : state.versions.slice(0, VISIBLE_VERSIONS);
  const selectedTag = state.selected?.kind === 'release' ? state.selected.version.tag : null;
  if (selectedTag && !list.some((v) => v.tag === selectedTag)) {
    list.push(state.versions.find((v) => v.tag === selectedTag));
  }

  for (const v of list) {
    const label = document.createElement('label');
    label.className = 'option';

    const input = document.createElement('input');
    input.type = 'radio';
    input.name = 'firmware';
    input.value = v.tag;
    input.checked = v.tag === selectedTag;
    input.addEventListener('change', () => select({ kind: 'release', version: v }));

    const body = document.createElement('span');
    body.className = 'option-body';
    const title = document.createElement('span');
    title.className = 'option-title';
    title.textContent = v.tag;
    if (v === state.versions[0]) title.append(badge('последняя'));
    if (v.prerelease) title.append(badge('тестовая', 'badge-pre'));
    const meta = document.createElement('span');
    meta.className = 'option-meta';
    meta.textContent = formatDate(v.date);
    body.append(title, meta);

    label.append(input, body);
    box.append(label);
  }

  const hidden = state.versions.length - VISIBLE_VERSIONS;
  $('show-all').hidden = state.showAll || hidden <= 0;
  $('show-all').textContent = `Показать все версии (${state.versions.length})`;
}

function badge(text, extra = '') {
  const el = document.createElement('span');
  el.className = `badge ${extra}`;
  el.textContent = text;
  return el;
}

async function loadVersions() {
  try {
    const res = await fetch('firmware/index.json', { cache: 'no-cache' });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const index = await res.json();
    state.versions = index.versions ?? [];
    if (index.repository) $('repo-link').href = index.repository;
  } catch (e) {
    log(`Список версий не загружен: ${e.message}`);
    state.versions = [];
  }

  const fromHash = decodeURIComponent(location.hash.slice(1));
  const initial = state.versions.find((v) => v.tag === fromHash) ?? state.versions[0];
  if (initial) select({ kind: 'release', version: initial });
  renderVersions();
}

function select(selection) {
  state.selected = selection;
  const token = ++state.token;
  const own = selection.kind === 'own';

  $('own-option').querySelector('input').checked = own;
  $('dropzone').hidden = !own;
  if (!own) history.replaceState(null, '', `#${encodeURIComponent(selection.version.tag)}`);

  renderDetails();
  renderDownloads();

  state.firmwareFailed = false;
  state.fw = null;
  if (own && !selection.file) {
    state.firmware = null;
  } else {
    state.firmware = own ? readOwnFile(selection.file) : fetchRelease(selection.version);
    state.firmware.then(
      (fw) => {
        if (token !== state.token) return;
        state.fw = fw;
        showCheck(`Файл проверен: ${formatSize(fw.bin.length)}`, 'ok');
        renderConfig();
        renderDownloads();
      },
      (e) => {
        if (token !== state.token) return;
        state.firmwareFailed = true;
        showCheck(e.message, 'error');
        renderConfig();
        updateButtons();
      },
    );
  }
  renderConfig();
  updateButtons();
}

async function prepareFirmware(name, buffer) {
  const fw = await loadFirmware(name, buffer);
  return { ...fw, config: findConfig(fw.bin) };
}

async function fetchRelease(version) {
  showCheck('Загружаю прошивку…');
  let res;
  try {
    res = await fetch(version.files.zip);
  } catch {
    throw new FirmwareError('Не удалось скачать прошивку — проверьте подключение к интернету');
  }
  if (!res.ok) throw new FirmwareError(`Не удалось скачать прошивку (HTTP ${res.status})`);
  return prepareFirmware(version.files.zip, await res.arrayBuffer());
}

async function readOwnFile(file) {
  showCheck('Проверяю файл…');
  return prepareFirmware(file.name, await file.arrayBuffer());
}

function showCheck(text, kind = '') {
  $('details').hidden = false;
  $('fw-check').textContent = text;
  $('fw-check').className = `fw-check ${kind}`;
}

function renderDetails() {
  const sel = state.selected;
  const release = sel?.kind === 'release' ? sel.version : null;

  $('details').hidden = !release && !sel?.file;
  $('fw-check').textContent = '';
  $('notes-box').hidden = !release?.notes;
  if (release?.notes) {
    $('notes-title').textContent = `Изменения в ${release.tag}`;
    $('notes').textContent = release.notes;
  }
  $('release-link').hidden = !release?.url;
  if (release?.url) $('release-link').querySelector('a').href = release.url;
}

function renderDownloads() {
  const box = $('downloads');
  box.replaceChildren();
  const release = state.selected?.kind === 'release' ? state.selected.version : null;

  if (state.fw?.config) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'download';
    button.textContent = `UF2 с настройками${release ? ` · ${release.tag}` : ''}`;
    button.addEventListener('click', downloadConfiguredUf2);
    box.append(button);
  }
  if (!release) {
    if (state.fw?.config) return;
    const span = document.createElement('span');
    span.className = 'muted';
    span.textContent = state.versions.length ? 'Выберите версию выше' : 'Опубликованных версий пока нет';
    box.append(span);
    return;
  }
  for (const [ext, title] of [['uf2', 'UF2'], ['hex', 'HEX'], ['zip', 'DFU .zip']]) {
    if (!release.files[ext]) continue;
    const a = document.createElement('a');
    a.href = release.files[ext];
    a.download = release.files[ext].split('/').pop();
    a.textContent = `${title} · ${release.tag}`;
    box.append(a);
  }
}

function downloadConfiguredUf2() {
  let image;
  try {
    image = buildImage(state.fw);
  } catch (e) {
    setStatus(e.message, 'error');
    return;
  }
  const release = state.selected?.kind === 'release' ? state.selected.version.tag : 'custom';
  const url = URL.createObjectURL(new Blob([makeUf2(image.bin)], { type: 'application/octet-stream' }));
  const a = document.createElement('a');
  a.href = url;
  a.download = `IMBA_TV-${release}-settings.uf2`;
  document.body.append(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 10000);
}

// ---------- Настройки ----------

function loadSaved() {
  try {
    return JSON.parse(localStorage.getItem(SETTINGS_KEY)) ?? {};
  } catch {
    return {};
  }
}

function saveValues() {
  try {
    localStorage.setItem(SETTINGS_KEY, JSON.stringify({ ...loadSaved(), ...state.values }));
  } catch {
    // без localStorage настройки просто не запомнятся
  }
}

// values: значения для формы; по умолчанию — из прошивки с учётом сохранённых в браузере
function renderConfig(values) {
  const form = $('config-form');
  form.replaceChildren();
  state.invalid.clear();
  const cfg = state.fw?.config;

  let empty = 'Выберите прошивку — здесь появятся её настройки.';
  if (state.firmwareFailed) empty = 'Настройки появятся, когда прошивка пройдёт проверку.';
  else if (state.firmware && !state.fw) empty = 'Загружаю настройки прошивки…';
  else if (state.fw) empty = 'Эта прошивка не поддерживает настройку и будет записана как есть.';
  $('config-empty').textContent = empty;
  $('config-empty').hidden = !!cfg;
  form.hidden = $('config-actions').hidden = $('config-note').hidden = !cfg;

  if (!cfg) {
    state.values = null;
    updateButtons();
    return;
  }
  state.values = values ?? mergeValues(cfg, readValues(state.fw.bin, cfg), loadSaved());

  for (const group of cfg.schema.groups) {
    const fieldset = document.createElement('fieldset');
    const legend = document.createElement('legend');
    legend.textContent = group.title;
    fieldset.append(legend);
    if (group.help) fieldset.append(textEl('p', 'group-help', group.help));

    const fields = document.createElement('div');
    fields.className = 'fields';
    for (const f of group.fields) fields.append(renderField(f, group));
    fieldset.append(fields);
    form.append(fieldset);
  }
  updateVisibility();
  updateButtons();
}

function textEl(tag, className, text) {
  const el = document.createElement(tag);
  el.className = className;
  el.textContent = text;
  return el;
}

function renderField(f, group) {
  const box = document.createElement('div');
  box.className = 'field';
  box.dataset.key = f.key;
  const id = `cfg-${f.key}`;
  const value = state.values[f.key];
  let input;

  if (f.type === 'bool') {
    const label = document.createElement('label');
    label.className = 'field-check';
    input = document.createElement('input');
    input.type = 'checkbox';
    input.id = id;
    input.checked = value;
    label.append(input, textEl('span', 'field-label', f.label));
    box.append(label);
  } else {
    const label = textEl('label', 'field-label', f.label);
    label.htmlFor = id;
    const row = document.createElement('div');
    row.className = 'field-row';

    if (f.options) {
      input = document.createElement('select');
      for (const o of f.options) input.append(new Option(o.label, o.value, false, o.value === value));
      row.append(input);
    } else if (f.widget === 'range') {
      input = document.createElement('input');
      input.type = 'range';
      const output = document.createElement('output');
      output.htmlFor = id;
      const show = () => { output.textContent = `${input.value}${f.unit ? ` ${f.unit}` : ''}`; };
      input.addEventListener('input', show);
      queueMicrotask(show);
      row.append(input, output);
    } else {
      input = document.createElement('input');
      input.type = 'number';
      input.inputMode = 'numeric';
      row.append(input);
      if (f.unit) row.append(textEl('span', 'unit', f.unit));
    }
    if (!f.options) {
      if (f.min != null) input.min = f.min;
      if (f.max != null) input.max = f.max;
      input.step = f.step ?? 1;
      input.value = value;
    }
    input.id = id;
    box.append(label, row);
  }

  if (f.help) box.append(textEl('span', 'field-help', f.help));
  const error = textEl('span', 'field-error', '');
  error.hidden = true;
  box.append(error);

  const onChange = () => {
    const raw = f.type === 'bool' ? input.checked : input.value;
    try {
      const next = checkValue(f, raw);
      if (group.require_one && f.type === 'bool' && !next &&
          !group.fields.some((g) => g !== f && state.values[g.key])) {
        input.checked = true;
        throw new FirmwareError('Нужно оставить хотя бы один вариант');
      }
      state.values[f.key] = next;
      state.invalid.delete(f.key);
      box.classList.remove('invalid');
      error.hidden = true;
      // подсказку «хотя бы один вариант» у соседних флажков тоже убираем
      if (f.type === 'bool') box.parentElement.querySelectorAll('.field-error').forEach((el) => { el.hidden = true; });
      saveValues();
    } catch (e) {
      error.textContent = e.message;
      error.hidden = false;
      if (f.type !== 'bool') {
        state.invalid.add(f.key);
        box.classList.add('invalid');
      }
    }
    updateVisibility();
    updateButtons();
  };
  input.addEventListener(f.type === 'bool' || f.options ? 'change' : 'input', onChange);
  return box;
}

function updateVisibility() {
  for (const f of state.fw?.config?.fields ?? []) {
    const box = $('config-form').querySelector(`[data-key="${f.key}"]`);
    const visible = isVisible(f, state.values);
    box.hidden = !visible;
    // скрытое поле с ошибкой не мешает прошивке: в образ попадёт последнее верное значение
    if (!visible && state.invalid.delete(f.key)) {
      box.classList.remove('invalid');
      box.querySelector('.field-error').hidden = true;
      box.querySelector('input').value = state.values[f.key];
    }
  }
}

function resetConfig() {
  const cfg = state.fw?.config;
  if (!cfg) return;
  try {
    const saved = loadSaved();
    for (const f of cfg.fields) delete saved[f.key];
    localStorage.setItem(SETTINGS_KEY, JSON.stringify(saved));
  } catch {
    // ignore
  }
  renderConfig(defaultValues(cfg));
}

// Образ для записи: прошивка с выбранными настройками и init-пакет с новой CRC
function buildImage(fw) {
  if (!fw.config) return fw;
  if (state.invalid.size) throw new FirmwareError('Исправьте ошибки в настройках');
  const values = state.fw === fw && state.values
    ? state.values
    : mergeValues(fw.config, readValues(fw.bin, fw.config), loadSaved());
  const bin = applyValues(fw.bin, fw.config, values);
  return { bin, dat: withImageCrc(fw.dat, bin), values };
}

function describeSettings(fw, values) {
  const changed = fw.config.fields.filter((f) => values[f.key] !== f.default);
  if (!changed.length) return 'по умолчанию';
  return changed.map((f) => {
    const v = values[f.key];
    const text = f.type === 'bool' ? (v ? 'да' : 'нет') : f.options?.find((o) => o.value === v)?.label ?? `${v}${f.unit ? ` ${f.unit}` : ''}`;
    return `${f.label}: ${text}`;
  }).join('; ');
}

function setOwnFile(file) {
  $('file-name').hidden = false;
  $('file-name').textContent = file.name;
  select({ kind: 'own', file });
}

// ---------- Прошивка ----------

function updateButtons() {
  $('flash').disabled = !hasSerial || state.busy || !state.firmware || state.firmwareFailed || state.invalid.size > 0;
}

function setBusy(busy) {
  state.busy = busy;
  document.querySelectorAll('input[name="firmware"], #file-pick, #config-form input, #config-form select, #config-reset')
    .forEach((el) => { el.disabled = busy; });
  updateButtons();
}

function portFilters() {
  return $('all-ports').checked ? {} : { filters: [{ usbVendorId: ADAFRUIT_VID }] };
}

function isBootloader(port) {
  const { usbVendorId, usbProductId } = port.getInfo();
  return usbVendorId === ADAFRUIT_VID && (usbProductId & 0x8000) === 0;
}

function describePort(port) {
  const { usbVendorId, usbProductId } = port.getInfo();
  if (usbVendorId == null) return 'порт без USB-идентификатора';
  const hex = (n) => n.toString(16).padStart(4, '0');
  return `USB ${hex(usbVendorId)}:${hex(usbProductId)}`;
}

// После перезагрузки в загрузчик появляется новое USB-устройство. Если доступ к нему
// уже выдавали раньше, браузер отдаст его сам, иначе нужен ещё один выбор порта.
function obtainBootloaderPort(oldPort) {
  return new Promise((resolve, reject) => {
    const started = Date.now();
    let finished = false;
    let timer;

    const finish = (fn, value) => {
      if (finished) return;
      finished = true;
      clearInterval(timer);
      $('pick-bootloader').hidden = true;
      $('cancel').hidden = true;
      state.cancel = null;
      fn(value);
    };

    timer = setInterval(async () => {
      const port = (await navigator.serial.getPorts()).find((p) => p !== oldPort && isBootloader(p));
      if (port) {
        log(`Загрузчик найден: ${describePort(port)}`);
        finish(resolve, port);
      } else if (Date.now() - started > 3000 && $('pick-bootloader').hidden) {
        setStatus('Плата перешла в режим загрузчика. Нажмите «Выбрать порт загрузчика» и выберите появившееся устройство.', 'wait');
        $('pick-bootloader').hidden = false;
        $('cancel').hidden = false;
      }
    }, 300);

    $('pick-bootloader').onclick = async () => {
      try {
        const port = await navigator.serial.requestPort(portFilters());
        log(`Выбран порт загрузчика: ${describePort(port)}`);
        finish(resolve, port);
      } catch (e) {
        if (e.name !== 'NotFoundError') finish(reject, e);
      }
    };
    state.cancel = () => finish(reject, new DOMException('Отменено', 'AbortError'));
  });
}

function explain(e) {
  if (e instanceof FirmwareError) return e.message;
  if (e.name === 'AbortError') return 'Прошивка отменена.';
  if (e.name === 'InvalidStateError' || e.name === 'NetworkError') {
    return 'Не удалось открыть порт. Закройте другие программы, которые его используют (Arduino IDE, монитор порта), переподключите плату и попробуйте снова.';
  }
  if (e instanceof DfuError) {
    return `${e.message}. Переподключите плату и попробуйте снова. Если не помогает — переведите её в режим загрузчика двойным замыканием RST на GND.`;
  }
  return `Ошибка: ${e.message}`;
}

async function flash() {
  if (state.busy || !state.firmware) return;
  const firmwarePromise = state.firmware;

  let port;
  try {
    port = await navigator.serial.requestPort(portFilters());
  } catch (e) {
    if (e.name === 'NotFoundError') {
      setStatus('Порт не выбран. Если платы нет в списке, проверьте, что кабель передаёт данные, или включите «Показывать все последовательные порты» в разделе «Дополнительно».');
    } else {
      setStatus(explain(e), 'error');
    }
    return;
  }

  setBusy(true);
  $('log').textContent = '';
  setProgress(null);

  try {
    const fw = await firmwarePromise;
    const image = buildImage(fw);
    log(`Прошивка: ${state.selected.kind === 'own' ? state.selected.file.name : state.selected.version.tag}, ${image.bin.length} байт`);
    if (fw.config) log(`Настройки: ${describeSettings(fw, image.values)}`);
    log(`Выбран порт: ${describePort(port)}`);

    if (!isBootloader(port)) {
      setStatus('Перезагружаю плату в режим загрузчика…', 'wait');
      log('Сброс через 1200 бод');
      await touch1200(port);
      port = await obtainBootloaderPort(port);
    }

    setStatus('Записываю прошивку… Не отключайте плату.', 'wait');
    setProgress(0, image.bin.length);
    const dfu = new SerialDfu(port, { log, onProgress: setProgress });
    await dfu.open();
    try {
      await dfu.flash(image.bin, image.dat);
    } finally {
      await dfu.close();
    }

    setStatus('Загрузчик применяет прошивку…', 'wait');
    await sleep(activateWaitTime(image.bin.length));
    log('Готово');
    setStatus('Готово! Прошивка записана, плата перезагрузилась. Если экран не включился, нажмите и удерживайте кнопку.', 'ok');
  } catch (e) {
    log(`Ошибка: ${e.message}`);
    setStatus(explain(e), 'error');
  } finally {
    setBusy(false);
  }
}

// ---------- Инициализация ----------

function init() {
  if (!hasSerial) $('unsupported').hidden = false;

  $('show-all').addEventListener('click', () => { state.showAll = true; renderVersions(); });
  $('own-option').querySelector('input').addEventListener('change', () => {
    select({ kind: 'own', file: state.selected?.kind === 'own' ? state.selected.file : null });
    if (!state.selected.file) $('file-input').click();
  });
  $('file-pick').addEventListener('click', () => $('file-input').click());
  $('file-input').addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (file) setOwnFile(file);
    e.target.value = '';
  });

  // Файл можно бросить на всю карточку выбора
  const zone = $('step1-title').closest('.card');
  zone.addEventListener('dragover', (e) => {
    if (state.busy) return;
    e.preventDefault();
    $('dropzone').hidden = false;
    $('dropzone').classList.add('dragover');
  });
  zone.addEventListener('dragleave', (e) => {
    if (!zone.contains(e.relatedTarget)) {
      $('dropzone').classList.remove('dragover');
      $('dropzone').hidden = state.selected?.kind !== 'own';
    }
  });
  zone.addEventListener('drop', (e) => {
    e.preventDefault();
    $('dropzone').classList.remove('dragover');
    const file = e.dataTransfer.files[0];
    if (file && !state.busy) setOwnFile(file);
    else $('dropzone').hidden = state.selected?.kind !== 'own';
  });

  $('flash').addEventListener('click', flash);
  $('config-reset').addEventListener('click', resetConfig);
  $('config-form').addEventListener('submit', (e) => e.preventDefault());
  $('cancel').addEventListener('click', () => state.cancel?.());
  window.addEventListener('beforeunload', (e) => {
    if (state.busy) e.preventDefault();
  });

  updateButtons();
  loadVersions();
}

init();
