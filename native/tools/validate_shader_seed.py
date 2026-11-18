#!/usr/bin/env python3
"""Validate a portable seed on real Metal, without opening a window.

The probe checks nonblack rendering and pause/resume. This wrapper additionally
requires every seeded configuration to compile before the warmup barrier and
can check discoveries from the latest session in an exported phone log.
"""
import argparse
import json
import os
from pathlib import Path
import sqlite3
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--seed', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--discovery-log', type=Path)
    parser.add_argument('--probe', type=Path,
                        default=ROOT/'native/build/aurora-integration/melee_runtime_probe')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    cache = args.output/'cache'
    cache.mkdir()
    with sqlite3.connect(args.seed.resolve().as_uri()+'?mode=ro', uri=True) as source:
        if source.execute('PRAGMA integrity_check').fetchone() != ('ok',):
            raise RuntimeError('Invalid shader seed')
        known = {row[0] & ((1 << 64)-1) for row in
                 source.execute('SELECT hash FROM pipeline_cache')}
        with sqlite3.connect(cache/'initial_pipeline_cache.db') as target:
            source.backup(target)
    expected = set()
    if args.discovery_log:
        rows = [json.loads(line) for line in args.discovery_log.read_text().splitlines()]
        latest = rows[-1]['session']
        expected = {int(e['hash'],16) for e in rows
                    if e['session'] == latest and e['event'] == 'discovered'}
        if not expected <= known:
            raise RuntimeError(f'Seed is missing {len(expected-known)} reported discoveries')
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0',
               MELEE_EXPECT_KNOWN_SHADERS=str(len(known)))
    # Prevent inherited test/save selectors from changing what this probe checks.
    for name in list(env):
        if name.startswith('MELEE_TEST_') or name.startswith('MELEE_AGENT_'):
            del env[name]
    with (args.output/'probe.log').open('w') as log:
        subprocess.run([str(args.probe.resolve()), str(args.disc.resolve()),
                        str(cache.resolve()), str((args.output/'frame.png').resolve())],
                       env=env, stdout=log, stderr=subprocess.STDOUT,
                       timeout=90, check=True)
    events = []
    for filename in ('shader-events-previous.jsonl', 'shader-events.jsonl'):
        path = cache/filename
        if path.exists():
            events.extend(json.loads(line) for line in path.read_text().splitlines())
    session = events[-1]['session']
    events = [e for e in events if e['session'] == session]
    barriers = [i for i, e in enumerate(events) if e['event'] == 'warmup_complete']
    if len(barriers) != 1 or any(e.get('droppedEvents') for e in events):
        raise RuntimeError('Incomplete shader preparation evidence')
    barrier = barriers[0]
    compiled = {int(e['hash'],16) for e in events[:barrier]
                if e['event'] == 'compiled' and e['fromCatalog']}
    discovered = {int(e['hash'],16) for e in events if e['event'] == 'discovered'}
    if compiled != known or discovered & known:
        raise RuntimeError('Seeded configurations were not all prepared before play')
    report = dict(knownPrepared=len(compiled),
                  latestReportedDiscoveriesCovered=len(expected),
                  preparationMilliseconds=events[barrier]['durationNs']/1e6,
                  newDuringProbe=len(discovered), eventsDropped=0)
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
