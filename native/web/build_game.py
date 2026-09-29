#!/usr/bin/env python3
"""Build the full-game Wasm source audit, Aurora renderer, or browser runtime."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
EM = ROOT / 'native/build/deps/emsdk/upstream/emscripten'
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--renderer', action='store_true')
p.add_argument('--game', action='store_true', help='Build full browser runtime (development checkpoint)')
args = p.parse_args()
if args.game: args.renderer = True
version = subprocess.check_output([str(EM/'emcc'), '--version'], text=True).splitlines()[0]
if '6.0.9' not in version:
    raise SystemExit(f'Expected pinned Emscripten 6.0.9, got {version}')
build = ROOT / ('native/build/wasm-renderer' if args.renderer else 'native/build/wasm-game')
subprocess.run([str(EM/'emcmake'), 'cmake', '-S', str(ROOT/'native/web/game'), '-B', str(build),
                '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
                f'-DMELEE_WEB_RENDERER={"ON" if args.renderer else "OFF"}'], check=True)
subprocess.run(['cmake', '--build', str(build), '--target',
                'melee_browser' if args.game else ('aurora_gx' if args.renderer else 'melee_web_game_objects'), '-j8'], check=True)

if args.game:
    import shutil
    for name in ('index.html','audio-worklet.js'):
        shutil.copy2(ROOT/'native/web/game'/name,build/name)
