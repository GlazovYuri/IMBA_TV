#!/usr/bin/env python3
"""Собирает сайт для GitHub Pages: страницу из web/ и прошивки всех релизов.

Файлы прошивок кладутся рядом со страницей: GitHub не отдаёт ассеты релизов
с CORS-заголовками, поэтому браузер не может скачать их напрямую.

Использование: build_site.py OUTPUT_DIR  (нужны gh CLI, GH_TOKEN и GITHUB_REPOSITORY)
"""
import json
import os
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXTENSIONS = ('zip', 'hex', 'uf2')
# Файлов прошивок десятки, и один сбой сети на любом из них ронял всю сборку сайта
GH_ATTEMPTS = 4


def gh(*args):
    for attempt in range(1, GH_ATTEMPTS + 1):
        result = subprocess.run(['gh', *args], capture_output=True, text=True)
        if result.returncode == 0:
            return result.stdout
        print(f'gh {" ".join(args[:3])}: попытка {attempt} из {GH_ATTEMPTS} не удалась: {result.stderr.strip()}',
              file=sys.stderr, flush=True)
        if attempt < GH_ATTEMPTS:
            time.sleep(5 * attempt)
    sys.exit(f'gh {" ".join(args)}: не удалось за {GH_ATTEMPTS} попытки')


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    repo = os.environ['GITHUB_REPOSITORY']

    shutil.rmtree(out, ignore_errors=True)
    shutil.copytree(os.path.join(ROOT, 'web'), out, ignore=shutil.ignore_patterns('test'))

    releases = json.loads(gh('release', 'list', '--repo', repo, '--limit', '1000', '--exclude-drafts',
                             '--json', 'tagName,publishedAt,isPrerelease'))
    releases.sort(key=lambda r: r['publishedAt'], reverse=True)

    versions = []
    for rel in releases:
        tag = rel['tagName']
        info = json.loads(gh('release', 'view', tag, '--repo', repo, '--json', 'body,url,assets'))
        names = {ext: a['name'] for a in info['assets'] for ext in EXTENSIONS if a['name'].endswith('.' + ext)}
        if 'zip' not in names:
            print(f'{tag}: нет DFU-пакета, пропускаю')
            continue

        target = os.path.join(out, 'firmware', tag)
        os.makedirs(target)
        args = ['release', 'download', tag, '--repo', repo, '--dir', target, '--clobber']
        for name in names.values():
            args += ['--pattern', name]
        gh(*args)

        versions.append({
            'tag': tag,
            'date': rel['publishedAt'],
            'prerelease': rel['isPrerelease'],
            'notes': info['body'].strip(),
            'url': info['url'],
            'files': {ext: f'firmware/{tag}/{name}' for ext, name in names.items()},
        })
        print(f'{tag}: {", ".join(sorted(names.values()))}')

    index = {'repository': f'https://github.com/{repo}', 'versions': versions}
    os.makedirs(os.path.join(out, 'firmware'), exist_ok=True)
    with open(os.path.join(out, 'firmware', 'index.json'), 'w') as f:
        json.dump(index, f, ensure_ascii=False, indent=1)
    print(f'Версий на сайте: {len(versions)}')


if __name__ == '__main__':
    main()
