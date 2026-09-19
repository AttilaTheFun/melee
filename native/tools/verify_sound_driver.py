#!/usr/bin/env python3
"""Exercise each US SEM bank using the game's own SSM filename table.

Requires an already-built test-sound-driver and a local game image. Each
process renders one second per command followed by key-off/drain; this is
bounded initial command coverage, not exhaustive command-path verification.
"""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--timeout', type=float, default=45)
    parser.add_argument('--binary', type=Path, default=ROOT / 'native/build/test-sound-driver')
    parser.add_argument('--output', type=Path, default=ROOT / 'native/build/sem-bank-results')
    args = parser.parse_args()
    source = (ROOT / 'src/melee/lb/lbaudio_ax.static.h').read_text()
    table = re.search(r'static const char\* ssm_files\[\]\s*=\s*\{(.*?)\};', source, re.S)
    assert table, 'SSM filename table not found'
    names = re.findall(r'"([^"]+\.ssm)"', table[1])
    assert len(names) == 55
    binary = args.binary.resolve()
    directory = args.output.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    commands = 0
    for index, name in enumerate(names):
        path = directory / f'{index:02d}-{name}.log'
        with path.open('w') as log:
            result = subprocess.run([str(binary), str(args.image.resolve()), str(index),
                                     f'audio/us/{name}'], cwd=ROOT, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=args.timeout)
        if result.returncode:
            raise SystemExit(f'Bank {index} ({name}) failed: {path}')
        summary = path.read_text().splitlines()[-1]
        match = re.search(r': (\d+) sounds,', summary)
        assert match, summary
        commands += int(match[1])
        print(summary, flush=True)
    assert commands == 4035
    print(f'Passed {len(names)} banks and {commands} bounded SEM command runs', flush=True)

if __name__ == '__main__':
    main()
