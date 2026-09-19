#!/usr/bin/env python3
"""Run original All-Star battles, rest portals and credits entry on native Metal.

Uses controlled KOs, stocks and portal placement with a fixed fixture seed.
Run sequentially with other gameplay/GPU/Simulator tests. Build the
melee_game_startup target first. This does not prove manual campaign completion.
"""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--disc', type=Path, default=ROOT/'native/build/simulator-test.ciso')
parser.add_argument('--seed', type=int, default=1)
args = parser.parse_args()
if not 0 <= args.seed <= 0xffffffff:
    parser.error('seed must fit an unsigned 32-bit integer')
build = ROOT/'native/build'
executable = (build/'aurora-integration/melee_game_startup').resolve(strict=True)
disc = args.disc.resolve(strict=True)
environment = {k: v for k, v in os.environ.items()
               if not k.startswith(('MELEE_TEST_ADVENTURE_', 'MELEE_TEST_ALLSTAR'))
               and k != 'MELEE_TEST_ESCAPE_COMPLETE'}
environment.update(MELEE_TEST_ALLSTAR='1', MELEE_TEST_ALLSTAR_CAMPAIGN='1',
                   MELEE_TEST_ALLSTAR_SEED=str(args.seed))
prefix = build/f'allstar-seed-{args.seed}'
log_path = prefix.with_suffix('.log')
print(f'Running All-Star; log: {log_path}', flush=True)
with log_path.open('w') as log:
    subprocess.run([str(executable), str(disc), str(build/'late-shader-cache'),
                    str(prefix)+'.png', '--classic-campaign'],
                   env=environment, stdout=log, stderr=subprocess.STDOUT,
                   check=True, timeout=2500)
if 'All-Star campaign through credits entry passed' not in log_path.read_text():
    raise SystemExit('The probe exited without proving the campaign checkpoint.')
print(f'All 13 rounds, 12 rest portals and credits entry passed: {prefix}.png')
