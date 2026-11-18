#!/usr/bin/env python3
"""Exercise real Metal cold/warm shader learning without a window or user saves."""
import argparse
import json
import os
from pathlib import Path
import sqlite3
import subprocess

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--disc', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--probe', type=Path, default=ROOT/'native/build/aurora-integration/melee_runtime_probe')
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
cache = a.output/'cache'
cache.mkdir()
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
report = []
known = set()
for name in ('cold', 'warm', 'incompatible'):
    with sqlite3.connect(cache/'pipeline_cache.db') as db:
        if name == 'incompatible':
            db.execute('UPDATE pipeline_cache SET config_version = 999999 WHERE hash = (SELECT hash FROM pipeline_cache LIMIT 1)')
        if name != 'cold':
            known = {row[0] & ((1 << 64)-1) for row in db.execute('SELECT hash FROM pipeline_cache WHERE config_version != 999999')}
    env['MELEE_EXPECT_KNOWN_SHADERS'] = str(len(known))
    with (a.output/f'{name}.log').open('w') as log:
        subprocess.run([str(a.probe.resolve()), str(a.disc.resolve()), str(cache.resolve()),
                        str((a.output/f'{name}.png').resolve())], env=env,
                       stdout=log, stderr=subprocess.STDOUT, timeout=90, check=True)
    lines = [json.loads(s) for s in (cache/'shader-events.jsonl').read_text().splitlines()]
    session = lines[-1]['session']
    events = [e for e in lines if e['session'] == session]
    barriers = [i for i, e in enumerate(events) if e['event'] == 'warmup_complete']
    assert len(barriers) == 1, events
    barrier = barriers[0]
    compiled = {int(e['hash'], 16) for e in events[:barrier] if e['event'] == 'compiled' and e['fromCatalog']}
    discoveries = {int(e['hash'], 16) for e in events if e['event'] == 'discovered'}
    assert not any(e.get('droppedEvents') for e in events)
    assert compiled == known, (len(compiled), len(known))
    assert not discoveries & known
    if name == 'cold':
        assert discoveries, 'The real game must discover shaders from an empty catalog.'
    report.append(dict(run=name, known=len(known), new=len(discoveries),
                       preparationMs=events[barrier]['durationNs']/1e6,
                       gameplayWaitMs=sum(e['durationNs'] for e in events[barrier+1:] if e['event']=='waited')/1e6))
(a.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
