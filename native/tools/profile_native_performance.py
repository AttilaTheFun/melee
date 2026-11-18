#!/usr/bin/env python3
"""Compare native shader scheduling on a windowless, fixed combat fixture.

Run sequentially with other GPU tests. Uses disposable GPU caches, never saves.
Requires melee_game_startup built with frame profiling support. OS/driver caches
are not reset; repeat with reversed order to check warm-cache/order effects.
"""
import argparse
import csv
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--seed-cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--reverse', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    reports = []
    modes = ['deferred', 'serial'] if args.reverse else ['serial', 'deferred']
    for mode in modes:
        cache = output / (mode + '-cache')
        cache.mkdir()
        for db in args.seed_cache.resolve().glob('*.db'):
            with sqlite3.connect(f'file:{db}?mode=ro', uri=True) as source, sqlite3.connect(cache / db.name) as target:
                source.backup(target)
        environment = {k: v for k, v in os.environ.items()
                       if not k.startswith(('MELEE_', 'AURORA_SERIAL_PIPELINES'))}
        environment.update(ASAN_OPTIONS='detect_leaks=0',
                           MELEE_FRAME_PROFILE=str(output / (mode + '.csv')))
        if mode == 'serial':
            environment['AURORA_SERIAL_PIPELINES'] = '1'
        started = time.monotonic()
        with (output / (mode + '.log')).open('w') as log:
            result = subprocess.run([str(ROOT / 'native/build/aurora-integration/melee_game_startup'),
                str(args.disc.resolve()), str(cache), str(output / (mode + '.png')), '--combat'],
                cwd=ROOT, env=environment, stdout=log, stderr=subprocess.STDOUT, timeout=180)
        if result.returncode:
            raise SystemExit(f'{mode} failed ({result.returncode}); see {output}')
        with (output / (mode + '.csv')).open() as file:
            rows = list(csv.DictReader(file))
        # The controlled fight is active by frame 2000. Exclude scene loading.
        game = [r for r in rows if int(r['frame']) >= 2000]
        intervals = sorted(float(r['interval_ms']) for r in game)
        report = dict(mode=mode, seconds=time.monotonic()-started, frames=len(rows),
            gameplayFrames=len(game), gameplayOver33ms=sum(x > 33.4 for x in intervals),
            gameplayMaxMs=max(intervals), gameplayP99Ms=intervals[int((len(intervals)-1)*.99)],
            pipelineWaitMs=sum(float(r['pipeline_wait_ms']) for r in rows),
            worst=sorted(rows, key=lambda r: float(r['interval_ms']), reverse=True)[:10])
        reports.append(report)
        (output / 'comparison.json').write_text(json.dumps(reports, indent=2) + '\n')
        print(json.dumps(report), flush=True)

if __name__ == '__main__':
    main()
