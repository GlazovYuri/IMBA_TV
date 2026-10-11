#!/usr/bin/env python3
"""Таблица глобальных и static-переменных прошивки для вкладки "Память" эмулятора.

gen_syms.py <бинарник> <выход.syms> <объектники прошивки...>

Имена берутся из объектников прошивки (чтобы не показывать переменные эмулятора),
адреса и размеры - из собранного бинарника. Эмулятор во время работы пересчитывает
адреса с учётом ASLR по метке emu_mem_anchor.
"""
import os
import re
import subprocess
import sys

DARWIN = sys.platform == "darwin"


def run(args):
    return subprocess.run(args, check=True, capture_output=True, text=True).stdout


def data_symbols(path):
    """[(имя, адрес, размер|None)] переменных в .data/.bss"""
    out = []
    if DARWIN:
        # 0000000100008000 (__DATA,__bss) non-external _foo
        rx = re.compile(r"^([0-9a-fA-F]+) \((__DATA\w*),(\w+)\) (?:\[[^\]]*\] )?(?:non-)?external (?:\[[^\]]*\] )?(\S+)")
        for line in run(["nm", "-m", path]).splitlines():
            m = rx.match(line)
            if m and m.group(3) not in ("__const", "__la_symbol_ptr", "__got", "__mod_init_func"):
                out.append((m.group(4), int(m.group(1), 16), None))
    else:
        for line in run(["nm", "-S", "--defined-only", path]).splitlines():
            f = line.split()
            if len(f) == 4 and f[2] in "bBdDvV":
                out.append((f[3], int(f[0], 16), int(f[1], 16)))
    return out


def all_symbols_sorted(path):
    """адреса всех символов бинарника (для оценки размеров на macOS)"""
    addrs = set()
    for line in run(["nm", "-n", path]).splitlines():
        f = line.split()
        if len(f) >= 3:
            try:
                addrs.add(int(f[0], 16))
            except ValueError:
                pass
    return sorted(addrs)


def demangle(names):
    if not names:
        return []
    raw = [n[1:] if DARWIN and n.startswith("_") else n for n in names]
    try:
        res = subprocess.run(["c++filt"], input="\n".join(raw) + "\n", capture_output=True, text=True, check=True)
        out = res.stdout.splitlines()
        if len(out) == len(raw):
            return out
    except (OSError, subprocess.CalledProcessError):
        pass
    return raw


def main():
    binary, dest, objs = sys.argv[1], sys.argv[2], sys.argv[3:]
    owner = {}
    for obj in objs:
        base = os.path.basename(obj)
        for ext in (".o",):
            if base.endswith(ext):
                base = base[: -len(ext)]
        for name, _, _ in data_symbols(obj):
            if name.startswith(("_ZGV", "__ZGV", "_ZTV", "__ZTV", "_ZTI", "__ZTI", "_ZTS", "__ZTS", "__odr_asan", "___odr_asan", "__asan", "___asan")):
                continue
            owner.setdefault(name, base)

    syms = data_symbols(binary)
    anchor = next((a for n, a, _ in syms if n.lstrip("_") == "emu_mem_anchor"), None)
    if anchor is None:
        sys.exit("gen_syms: в бинарнике нет emu_mem_anchor")

    picked = [(n, a, s) for n, a, s in syms if n in owner]
    if DARWIN:
        addrs = all_symbols_sorted(binary)
        import bisect

        fixed = []
        for n, a, _ in picked:
            i = bisect.bisect_right(addrs, a)
            size = (addrs[i] - a) if i < len(addrs) else 16
            fixed.append((n, a, min(size, 8192)))
        picked = fixed

    picked.sort(key=lambda t: (owner[t[0]], t[1]))
    names = demangle([n for n, _, _ in picked])
    with open(dest, "w") as f:
        f.write("#anchor\t%x\n" % anchor)
        for (n, a, s), dn in zip(picked, names):
            if s:
                f.write("%x\t%d\t%s\t%s\t%s\n" % (a, s, n, dn, owner[n]))
    print("gen_syms: %d переменных прошивки" % len(picked))


if __name__ == "__main__":
    main()
