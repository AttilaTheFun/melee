#!/usr/bin/env python3
"""Build the headless Apple audio integration test with the real native mixer."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/audio-test'
BUILD.mkdir(parents=True, exist_ok=True)
objects = []
for source in sys.argv[1:]:
    obj = BUILD / (Path(source).stem + '.o')
    if str(obj) in objects:
        raise SystemExit(f'Duplicate audio test object: {obj}')
    subprocess.run(['xcrun', 'clang', '-target', 'arm64-apple-macos13',
                    '-std=gnu11', '-DMELEE_NATIVE', '-Iinclude', '-I../src',
                    '-I../extern/dolphin/include', '-c', source, '-o', str(obj)],
                   cwd=ROOT, check=True)
    objects.append(str(obj))
subprocess.run(['xcrun', 'swiftc', '-target', 'arm64-apple-macos13',
                '-swift-version', '5', '-module-cache-path', 'build/audio-module-cache',
                '-Iinclude', '-I../extern/dolphin/include', '-I../src',
                '-Xcc', '-DMELEE_NATIVE', '-import-objc-header', 'apple/AudioBridge.h',
                'apple/AudioOutput.swift', 'tests/test_audio_output.swift', *objects,
                '-o', 'build/test-audio-output'], cwd=ROOT, check=True)
