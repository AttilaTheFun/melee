#!/usr/bin/env python3
"""Build the offscreen native model/Metal geometry verifier (no host UI)."""
from pathlib import Path
import subprocess
from sources import SOURCES
ROOT = Path(__file__).resolve().parents[2]
build = ROOT / 'native/build/renderer'
build.mkdir(parents=True, exist_ok=True)
sdk = subprocess.check_output(['xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
objects = []
for source in SOURCES + ['native/tools/render_bridge.c']:
    obj = build / Path(source).with_suffix('.o')
    obj.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['xcrun', 'clang', '-target', 'arm64-apple-macos13', '-isysroot', sdk,
        '-std=gnu11', '-O2', '-DMELEE_NATIVE', '-Inative/include', '-Iextern/dolphin/include', '-Isrc',
        '-c', source, '-o', str(obj)], cwd=ROOT, check=True)
    objects.append(str(obj))
subprocess.run(['xcrun', 'swiftc', '-target', 'arm64-apple-macos13', '-sdk', sdk,
    '-module-cache-path', str(build / 'module-cache'), '-swift-version', '5', '-parse-as-library',
    '-import-objc-header', 'native/tools/RenderBridge.h', '-Xcc', '-DMELEE_NATIVE', '-Inative/include',
    'native/apple/MetalGeometryRenderer.swift', 'native/tools/RenderModel.swift', *objects,
    '-Xlinker', '-dead_strip', '-o', str(build / 'render-model')], cwd=ROOT, check=True)
print(build / 'render-model')
