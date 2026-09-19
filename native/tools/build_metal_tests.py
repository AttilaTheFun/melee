#!/usr/bin/env python3
"""Build synthetic offscreen GPU tests; execute the output with Metal access."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
build = ROOT / 'native/build/metal-tests'
build.mkdir(parents=True, exist_ok=True)
sdk = subprocess.check_output(['xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
subprocess.run(['xcrun', 'swiftc', '-target', 'arm64-apple-macos13', '-sdk', sdk,
    '-module-cache-path', str(build / 'module-cache'), '-swift-version', '5', '-parse-as-library',
    'native/apple/MetalGeometryRenderer.swift', 'native/tests/TestMetalRenderer.swift',
    '-o', str(build / 'test-metal')], cwd=ROOT, check=True)
print(build / 'test-metal')
