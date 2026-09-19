#!/usr/bin/env python3
"""Build/link the full game against the configured, pinned Aurora GX backend.

This does not launch the executable. Even a successful link is not evidence of
playability or a safe entry point. Configure native/aurora first, as described
in native/README.md. This currently uses the macOS Aurora build, not iOS.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
from prepare_fonts import DEFAULT_DOL, prepare

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'native/build/aurora-integration')
    parser.add_argument('--jobs', type=int, default=8)
    args = parser.parse_args()
    build = args.build_dir.resolve()
    if not (build / 'CMakeCache.txt').is_file():
        parser.error('Configure the Aurora CMake project before running this audit')
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    cmake = shutil.which('cmake')
    if not cmake:
        parser.error('cmake was not found')
    if DEFAULT_DOL.is_file():
        prepare()
    binary = build / 'melee_game_link_audit'
    binary.unlink(missing_ok=True)
    command = [cmake, '--build', str(build), '--target', 'melee_game_link_audit',
               '--parallel', str(args.jobs)]
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    log = result.stdout + result.stderr
    (build / 'full-game-link.log').write_text(log)
    linked = result.returncode == 0
    undefined = re.findall(r'^  "([^"]+)", referenced from:', log, re.MULTILINE)
    report = {'command': command, 'linked': linked, 'returncode': result.returncode,
              'link_attempted': linked or 'Undefined symbols for architecture' in log,
              'undefined_symbols': undefined, 'runtime_tested': False,
              'graphics_abi': 'MELEE_AURORA', 'platform': 'macOS'}
    (build / 'full-game-link.json').write_text(json.dumps(report, indent=2) + '\n')
    if not linked:
        binary.unlink(missing_ok=True)
    print(f"Aurora full game: {'linked' if linked else 'incomplete'}, {len(undefined)} reported unresolved symbols; not executed")
    print(build / 'full-game-link.log')
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
