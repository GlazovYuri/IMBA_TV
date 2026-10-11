#!/usr/bin/env python3
"""Генерирует IMBA_TV/config_gen.h из IMBA_TV/config.json.

config.json описывает настройки прошивки: из него получаются структура fw_config_t,
значения по умолчанию, проверка значений и описание формы, которое встраивается
в прошивку для веб-конфигуратора. После правки config.json запустите:

    python3 tools/gen_config.py

С ключом --check только проверяет, что config_gen.h соответствует config.json.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(ROOT, 'IMBA_TV', 'config.json')
TARGET = os.path.join(ROOT, 'IMBA_TV', 'config_gen.h')

TYPES = {
    'bool': ('uint8_t', 1, 0, 1),
    'u8': ('uint8_t', 1, 0, 0xFF),
    'u16': ('uint16_t', 2, 0, 0xFFFF),
}


def fail(message):
    sys.exit(f'config.json: {message}')


def load():
    with open(SOURCE, encoding='utf-8') as f:
        schema = json.load(f)

    seen = {}
    offset = 0
    for group in schema['groups']:
        if not group.get('title'):
            fail('у группы нет title')
        for field in group['fields']:
            key = field.get('key', '')
            if not re.fullmatch(r'[a-z][a-z0-9_]*', key):
                fail(f'недопустимый key "{key}"')
            if key in seen:
                fail(f'повторяется key "{key}"')
            if field.get('type') not in TYPES:
                fail(f'{key}: тип должен быть одним из {", ".join(TYPES)}')
            if not field.get('label'):
                fail(f'{key}: нет label')

            _, size, lo, hi = TYPES[field['type']]
            default = field.get('default')
            if field['type'] == 'bool':
                if not isinstance(default, bool):
                    fail(f'{key}: default должен быть true или false')
            else:
                if not isinstance(default, int) or isinstance(default, bool):
                    fail(f'{key}: default должен быть целым числом')
                lo = field.get('min', lo)
                hi = field.get('max', hi)
                if 'options' in field:
                    values = [o['value'] for o in field['options']]
                    if len(set(values)) != len(values):
                        fail(f'{key}: повторяются значения options')
                    for o in field['options']:
                        if not re.fullmatch(r'[A-Z0-9_]+', o.get('id', '')) or not o.get('label'):
                            fail(f'{key}: у каждого варианта нужны id (A-Z, 0-9, _) и label')
                        if not TYPES[field['type']][2] <= o['value'] <= TYPES[field['type']][3]:
                            fail(f'{key}: значение {o["value"]} не помещается в {field["type"]}')
                    if default not in values:
                        fail(f'{key}: default не входит в options')
                elif not lo <= default <= hi:
                    fail(f'{key}: default вне диапазона {lo}..{hi}')

            for dep_key, dep_value in field.get('depends', {}).items():
                if dep_key not in seen:
                    fail(f'{key}: depends ссылается на "{dep_key}", который должен быть объявлен раньше')
                dep_type = seen[dep_key]['type']
                if (dep_type == 'bool') != isinstance(dep_value, bool):
                    fail(f'{key}: depends.{dep_key} не того типа')

            field['offset'] = offset
            field['size'] = size
            offset += size
            seen[key] = field
    return schema, list(seen.values()), offset


def c_value(field, value):
    return str(int(value))


def sanitize_lines(field):
    key = field['key']
    _, _, type_lo, type_hi = TYPES[field['type']]
    default = c_value(field, field['default'])
    if field['type'] == 'bool':
        return [f'  if(c.{key} > 1) c.{key} = 1;']
    if 'options' in field:
        cases = ' '.join(f'case {o["value"]}:' for o in field['options'])
        return [f'  switch(c.{key}) {{ {cases} break; default: c.{key} = {default}; }}']
    checks = []
    if field.get('min', type_lo) > type_lo:
        checks.append(f'c.{key} < {field["min"]}')
    if field.get('max', type_hi) < type_hi:
        checks.append(f'c.{key} > {field["max"]}')
    return [f'  if({" || ".join(checks)}) c.{key} = {default};'] if checks else []


def c_string(text, width=96):
    escaped = [{'"': '\\"', '\\': '\\\\'}.get(ch, ch) for ch in text]
    lines = [''.join(escaped[i:i + width]) for i in range(0, len(escaped), width)]
    return '\n'.join(f'  "{line}"' for line in lines)


def render(schema, fields, size):
    out = [
        '#pragma once',
        '',
        '// Сгенерировано tools/gen_config.py из config.json, не править руками.',
        '// После изменения config.json запустите: python3 tools/gen_config.py',
        '',
        '#include <stdint.h>',
        '',
        '// Настройки прошивки. Значения записывает веб-конфигуратор перед прошивкой.',
        'struct __attribute__((packed)) fw_config_t',
        '{',
    ]
    for f in fields:
        out.append(f'  {TYPES[f["type"]][0]} {f["key"]};  // {f["label"]}')
    out += ['};', '', f'static_assert(sizeof(fw_config_t) == {size}, "config.json и fw_config_t разошлись");', '']

    out.append('#define FW_CONFIG_DEFAULTS { ' + ', '.join(c_value(f, f['default']) for f in fields) + ' }')
    out.append('')

    enums = [(f, o) for f in fields for o in f.get('options', [])]
    if enums:
        out += ['enum', '{']
        for f, o in enums:
            out.append(f'  CONFIG_{f["key"].upper()}_{o["id"]} = {o["value"]},')
        out += ['};', '']

    out += [
        '// Значения вне допустимых заменяются значениями по умолчанию',
        'static inline void configSanitize(fw_config_t& c)',
        '{',
    ]
    for f in fields:
        out += sanitize_lines(f)
    out += ['}', '']

    embedded = json.dumps(schema, ensure_ascii=False, separators=(',', ':'))
    out += [
        '// Описание настроек для веб-конфигуратора: config.json с адресами полей',
        '#define FW_CONFIG_SCHEMA \\',
        c_string(embedded).replace('\n', ' \\\n'),
        '',
    ]
    return '\n'.join(out)


def main():
    schema, fields, size = load()
    text = render(schema, fields, size)

    if '--check' in sys.argv:
        with open(TARGET, encoding='utf-8') as f:
            if f.read() != text:
                sys.exit('config_gen.h устарел: запустите python3 tools/gen_config.py')
        print('config_gen.h соответствует config.json')
        return

    with open(TARGET, 'w', encoding='utf-8') as f:
        f.write(text)
    print(f'{os.path.relpath(TARGET, ROOT)}: {len(fields)} настроек, {size} байт')


if __name__ == '__main__':
    main()
