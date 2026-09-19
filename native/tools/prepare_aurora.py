#!/usr/bin/env python3
"""Fetch the pinned source-level GX backend; no app or GPU is started."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'native/build/deps/aurora'
URL = 'https://github.com/encounter/aurora.git'
REVISION = '749d6ee7a22bdfab78c8ece9047bca5d79aa72ca'
if not SOURCE.exists():
    SOURCE.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['git', 'init', str(SOURCE)], check=True)
    subprocess.run(['git', '-C', str(SOURCE), 'remote', 'add', 'origin', URL], check=True)
    subprocess.run(['git', '-C', str(SOURCE), 'fetch', '--depth', '1', 'origin', REVISION], check=True)
    subprocess.run(['git', '-C', str(SOURCE), 'checkout', '--detach', REVISION], check=True)
revision = subprocess.check_output(['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'], text=True).strip()
patch = ROOT / 'native/aurora/aurora.patch'
expected = patch.read_bytes()
actual = subprocess.check_output(['git', '-C', str(SOURCE), 'diff', '--binary'])
staged = subprocess.check_output(['git', '-C', str(SOURCE), 'diff', '--cached'])
untracked = subprocess.check_output(['git', '-C', str(SOURCE), 'ls-files', '--others', '--exclude-standard'])
if revision != REVISION or staged or untracked or actual not in (b'', expected):
    raise SystemExit('Aurora checkout differs from the pin and reviewed patch; preserve/review it before rebuilding.')
if not actual:
    subprocess.run(['git', '-C', str(SOURCE), 'apply', str(patch)], check=True)
print(SOURCE)
print('Aurora MIT license: ' + str(SOURCE / 'LICENSE'))
