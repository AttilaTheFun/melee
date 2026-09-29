#!/usr/bin/env python3
"""Build the asset-free browser prototype with the project's shared C sources."""
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

NATIVE = Path(__file__).resolve().parents[1]
OUT = NATIVE / 'build/web'
EMCC = Path(os.environ.get('EMCC', NATIVE / 'build/deps/emsdk/upstream/emscripten/emcc'))
OUT.mkdir(parents=True, exist_ok=True)
version = subprocess.check_output([str(EMCC), '--version'], text=True).splitlines()[0]
if '6.0.9' not in version:
    raise SystemExit(f'Expected Emscripten 6.0.9; got {version}')
# Keep the decoder/animation source set identical to the native model verifier.
dry = subprocess.check_output(['make', '-n', '-B', 'build/verify-model'], cwd=NATIVE, text=True)
command = next(line for line in dry.splitlines() if 'tools/verify_model.c' in line)
sources = [str((NATIVE / p).resolve()) for p in shlex.split(command)
           if p.endswith('.c') and p != 'tools/verify_model.c']
sources += [str(NATIVE / p) for p in ['web/scene.c', 'src/disc.c', 'src/gx_pixel.c',
                                    'src/os_interrupt.c', '../src/sysdolphin/baselib/state.c']]
exports = ['malloc', 'free', 'web_error', 'web_close_scene', 'web_open_disc',
           'web_load_fighter', 'web_step', 'web_part_count', 'web_part_info',
           'web_omitted_layers', 'web_duration']
args = [str(EMCC), '-DMELEE_NATIVE', '-Iinclude', '-Itools', '-I../extern/dolphin/include',
        '-I../src', '-std=gnu11', '-O2', '-g', '-sASSERTIONS=1', '-sALLOW_MEMORY_GROWTH=1',
        '-sINITIAL_MEMORY=67108864', '-sMAXIMUM_MEMORY=536870912', '-sSTACK_SIZE=1048576',
        '-sMODULARIZE=1', '-sEXPORT_ES6=1', '-sENVIRONMENT=web,worker,node',
        '-sFORCE_FILESYSTEM=1', '-lworkerfs.js', '-lnodefs.js',
        '-sEXPORTED_FUNCTIONS=' + json.dumps(['_' + x for x in exports]),
        '-sEXPORTED_RUNTIME_METHODS=' + json.dumps(['ccall', 'UTF8ToString', 'FS', 'WORKERFS', 'NODEFS', 'HEAPU8']),
        *sources, '-o', str(OUT / 'melee.mjs')]
subprocess.run(args, cwd=NATIVE, check=True)
for name in ['index.html', 'worker.js', 'renderer.js', 'app.js', 'scene-data.js']:
    if (NATIVE / 'web' / name).exists():
        shutil.copy2(NATIVE / 'web' / name, OUT / name)
(OUT / 'build.json').write_text(json.dumps({'compiler': version, 'sources': sources}, indent=2) + '\n')
print(OUT)
