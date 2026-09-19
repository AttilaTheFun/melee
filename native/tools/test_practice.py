#!/usr/bin/env python3
"""Exercise Training menus or a Classic target-course checkpoint on native Metal.

Run sequentially with other GPU/Simulator tests. The target fixture uses original
target damage callbacks to complete the course; it does not test manual aiming.
"""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
characters = ['captain', 'donkey', 'fox', 'gamewatch', 'kirby', 'bowser', 'link',
              'luigi', 'mario', 'marth', 'mewtwo', 'ness', 'peach', 'pikachu',
              'iceclimbers', 'jigglypuff', 'samus', 'yoshi', 'zelda', 'sheik',
              'falco', 'younglink', 'doctor', 'roy', 'pichu', 'ganondorf']
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('mode', choices=['training', 'targets', 'homerun'])
parser.add_argument('--character', choices=characters, default='mario')
parser.add_argument('--disc', type=Path, default=ROOT/'native/build/simulator-test.ciso')
args = parser.parse_args()
build = ROOT/'native/build'
executable = (build/'aurora-integration/melee_game_startup').resolve(strict=True)
disc = args.disc.resolve(strict=True)
environment = {k: v for k, v in os.environ.items()
               if not k.startswith(('MELEE_TEST_ADVENTURE_', 'MELEE_TEST_ALLSTAR'))
               and k not in ('MELEE_TEST_ESCAPE_COMPLETE', 'MELEE_TEST_TARGET_COURSE', 'MELEE_TEST_TRAINING', 'MELEE_TEST_HOMERUN')}
if args.mode == 'homerun':
    environment['MELEE_TEST_HOMERUN'] = '1'
    environment['MELEE_TEST_HOMERUN_COMPLETE'] = '1'
    name = 'homerun-current'
elif args.mode == 'training':
    environment['MELEE_TEST_TRAINING'] = '1'
    name = 'training-current'
else:
    environment['MELEE_TEST_TARGET_COURSE'] = str(characters.index(args.character))
    name = f'target-course-{args.character}'
prefix = build/name
log_path = prefix.with_suffix('.log')
print(f'Running {args.mode}; log: {log_path}', flush=True)
with log_path.open('w') as log:
    subprocess.run([str(executable), str(disc), str(build/'late-shader-cache'),
                    str(prefix)+'.png', '--classic-race-entry'], env=environment,
                   stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
marker = 'Home Run timeout-to-character-select passed' if args.mode == 'homerun' else 'Training result: menu=1 speed-change=1 closed=1' if args.mode == 'training' else 'cleared=10 mode=3 scene=25'
if marker not in log_path.read_text():
    raise SystemExit('The probe exited without the required mode checkpoint.')
print(f'Practice checkpoint passed: {prefix}.png')
