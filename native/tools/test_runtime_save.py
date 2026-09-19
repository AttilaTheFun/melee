#!/usr/bin/env python3
"""Verify original-game save creation and changed-setting and unlock reload across processes.

Run sequentially with other native GPU/Simulator tests. Build melee_runtime_probe
first. Uses a disposable card; never reads or changes the app's user save.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--disc', type=Path, default=ROOT/'native/build/simulator-test.ciso')
parser.add_argument('--output', type=Path, help='Keep this run’s logs and screenshots in a separate directory')
args = parser.parse_args()
disc = args.disc.resolve(strict=True)
build = ROOT/'native/build'
output = args.output or build
output.mkdir(parents=True, exist_ok=True)
probe = (build/'aurora-integration/melee_runtime_probe').resolve(strict=True)
with tempfile.TemporaryDirectory(prefix='save-roundtrip-', dir=build) as temporary:
    card = Path(temporary)/'SlotA.card'
    for phase in ['write', 'reload']:
        environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=0',
                           MELEE_TEST_CARD=str(card), MELEE_TEST_SAVE_EXPECT='0',
                           MELEE_CARD_TRACE='1', MELEE_TEST_SAVE_UNLOCK='1')
        environment.pop('MELEE_TEST_SAVE_WRITE', None)
        if phase == 'write':
            environment['MELEE_TEST_SAVE_WRITE'] = '1'
        with (output/f'save-roundtrip-{phase}.log').open('w') as log:
            subprocess.run([str(probe), str(disc), str(build/'transition-probe-cache'),
                            str(output/f'save-roundtrip-{phase}.png'), '--audio'],
                           env=environment, stdout=log, stderr=subprocess.STDOUT, check=True)
        if not card.exists() or card.stat().st_size <= 8148:
            raise RuntimeError('The game did not write data to the test memory card.')
print('Original game save: changed rumble setting and Luigi unlock restored in a fresh process.')
