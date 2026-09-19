#!/usr/bin/env python3
"""Run a controlled native Adventure checkpoint with real disc assets and Metal.

Run sequentially with other gameplay, GPU and Simulator tests. These fixtures
use scripted KOs/stocks and course placement; they do not prove manual traversal.
Build the melee_game_startup target first. The default disc is the local test copy.
"""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
CHECKPOINTS = {'kirby': 32, 'starfox': 40, 'fzero': 56, 'onett': 64,
               'icicle': 72, 'wireframes': 80, 'finale': 88}
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('checkpoint', choices=CHECKPOINTS)
parser.add_argument('--disc', type=Path, default=ROOT/'native/build/simulator-test.ciso')
args = parser.parse_args()
build = ROOT/'native/build'
executable = (build/'aurora-integration/melee_game_startup').resolve(strict=True)
disc = args.disc.resolve(strict=True)
# Isolate fixture selection from unrelated environment flags in the caller.
environment = {k: v for k, v in os.environ.items()
               if not k.startswith(('MELEE_TEST_ADVENTURE_', 'MELEE_TEST_ALLSTAR'))
               and k != 'MELEE_TEST_ESCAPE_COMPLETE'}
environment.update(MELEE_TEST_ESCAPE_COMPLETE='1', MELEE_TEST_ADVENTURE_KIRBY='1',
                   MELEE_TEST_ADVENTURE_CHECKPOINT_SCENE=str(CHECKPOINTS[args.checkpoint]))
if args.checkpoint in ('starfox', 'fzero'):
    environment['MELEE_TEST_ADVENTURE_STARFOX'] = '1'
if args.checkpoint == 'fzero':
    environment['MELEE_TEST_ADVENTURE_FZERO'] = '1'
    environment['MELEE_TEST_ADVENTURE_ONETT_ENTRY'] = '1'
prefix = build/f'adventure-checkpoint-{args.checkpoint}'
print(f'Running {args.checkpoint}; log: {prefix}.log', flush=True)
with Path(str(prefix)+'.log').open('w') as log:
    subprocess.run([str(executable), str(disc), str(build/'late-shader-cache'),
                    str(prefix)+'.png', '--adventure-brinstar-escape'],
                   env=environment, stdout=log, stderr=subprocess.STDOUT,
                   check=True, timeout=1300)
print(f'Checkpoint passed. Inspect the actual framebuffer: {prefix}.png')
