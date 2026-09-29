#!/usr/bin/env python3
"""Reject casts that conceal incompatible item callback signatures on Wasm."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
EMCC = ROOT/'native/build/deps/emsdk/upstream/emscripten/emcc'
for name in ('itarwinglaser.c', 'ittarucann.c'):
    subprocess.run([str(EMCC), '-DMELEE_NATIVE', '-Inative/include',
                    '-Iextern/dolphin/include', '-Isrc', '-std=gnu11',
                    '-Werror=cast-function-type-strict',
                    '-Werror=incompatible-function-pointer-types',
                    '-fsyntax-only', 'src/melee/it/kinds/'+name],
                   cwd=ROOT, check=True)
print('PASS strict Arwing/barrel item callback table types')
