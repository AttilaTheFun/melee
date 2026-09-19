#!/usr/bin/env python3
"""Exercise Apple input integration without opening a window."""
from pathlib import Path
import subprocess
from sources import SOURCES

ROOT = Path(__file__).resolve().parents[2]
build = ROOT / 'native/build/apple-tests'
build.mkdir(parents=True, exist_ok=True)
sdk = subprocess.check_output(['xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
objects = []
for source in SOURCES:
    obj = build / Path(source).with_suffix('.o')
    obj.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['xcrun', 'clang', '-target', 'arm64-apple-macos13', '-isysroot', sdk,
        '-std=gnu11', '-DMELEE_NATIVE', '-Inative/include', '-Iextern/dolphin/include', '-Isrc', '-c',
        source, '-o', str(obj)], cwd=ROOT, check=True)
    objects.append(str(obj))
exe = build / 'test-apple'
subprocess.run(['xcrun', '--sdk', 'macosx', 'swiftc', '-target', 'arm64-apple-macos13', '-sdk', sdk,
    '-module-cache-path', str(build / 'module-cache'), '-swift-version', '5',
        '-Xlinker', '-dead_strip',
    '-DMELEE_TEST', '-parse-as-library', '-import-objc-header', 'native/apple/Bridge.h',
    '-Xcc', '-DMELEE_NATIVE', '-Inative/include', '-Iextern/dolphin/include',
    'native/apple/MeleeNative.swift', 'native/apple/MetalGeometryRenderer.swift', 'native/tests/TestApple.swift', *objects,
    '-o', str(exe)], cwd=ROOT, check=True)
subprocess.run([str(exe)], check=True)
