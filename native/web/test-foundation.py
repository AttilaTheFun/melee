#!/usr/bin/env python3
"""Run the real timing, command and JPEG regression tests compiled to Wasm."""
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
NATIVE = ROOT/'native'
EMCC = NATIVE/'build/deps/emsdk/upstream/emscripten/emcc'
OUT = NATIVE/'build/web-foundation'
OUT.mkdir(parents=True, exist_ok=True)
for name in ['alarm', 'vi-backend', 'command', 'jpeg-runtime', 'card-store', 'movie-stop', 'item-spawn-tables', 'arwing-laser-callbacks']:
    target = f'build/test-{name}'
    dry = subprocess.check_output(['make', '-n', '-B', target], cwd=NATIVE, text=True)
    line = next(line for line in dry.splitlines() if f'-o {target}' in line)
    original = shlex.split(line)[2:] # xcrun clang
    args = [s for s in original if not s.startswith(('-fsanitize=', '-Wl,'))]
    args[args.index('-o')+1] = str(OUT/f'{name}.js')
    subprocess.run([str(EMCC), *args, '-D_POSIX_C_SOURCE=200809L', '-pthread', '-sPROXY_TO_PTHREAD=1',
                    '-sPTHREAD_POOL_SIZE=4', '-sEXIT_RUNTIME=1', '-sASSERTIONS=2',
                    '-sSTACK_SIZE=1048576', '-Wno-limited-postlink-optimizations'], cwd=NATIVE, check=True)
    subprocess.run(['node', str(OUT/f'{name}.js')], check=True, timeout=90)
    print(f'PASS Wasm {name}', flush=True)

subprocess.run([str(EMCC), '-std=c11', '-O2', '-D_POSIX_C_SOURCE=200809L',
                '-DMELEE_NATIVE', f'-I{NATIVE}/include',
                str(NATIVE/'web/test-disc-fonts.c'), str(NATIVE/'src/disc.c'),
                '-o', str(OUT/'disc-fonts.js')], check=True)
subprocess.run(['node', str(OUT/'disc-fonts.js')], check=True, timeout=30)
