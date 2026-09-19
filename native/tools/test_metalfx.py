#!/usr/bin/env python3
"""Build and run real Mac MetalFX checks without opening a window.

Run sequentially with other GPU tests. Optional game image must be 640x480.
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--image', type=Path)
args = parser.parse_args()
build = ROOT/'native/build'
exe = build/'test-metalfx'
subprocess.run(['xcrun', 'swiftc', '-swift-version', '5', '-parse-as-library',
                '-module-cache-path', str(build/'metalfx-test-cache'),
                str(ROOT/'native/apple/MetalFXRenderer.swift'),
                str(ROOT/'native/tests/test_metalfx.swift'), '-o', str(exe)], check=True)
command = [str(exe)]
if args.image:
    command += [str(args.image.resolve(strict=True)), str(build/'metalfx-game')]
subprocess.run(command, check=True)
