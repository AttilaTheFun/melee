#!/usr/bin/env python3
"""Print exported hitch stacks using a matching, unstripped runtime binary.

Example: symbolicate_hitches.py profile.jsonl --binary native/build/aurora-iphoneos/libmelee_game_runtime.dylib
UUID matching prevents accidentally resolving addresses against a newer build.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('logs', nargs='+', type=Path)
parser.add_argument('--binary', required=True, type=Path)
args = parser.parse_args()
identity = subprocess.check_output(['dwarfdump', '--uuid', str(args.binary)], text=True)
uuids = {s.replace('-', '').lower() for s in re.findall(r'UUID: ([0-9A-Fa-f-]+)', identity)}
cache = {}
matched = False
for log in args.logs:
    for line in log.read_text().splitlines():
        event = json.loads(line)
        if event.get('type') != 'hitch':
            continue
        print(f"\nHitch at {event['uptimeNs']/1e9:.3f}s: {event['gapMs']:.1f}ms without a frame, phase={event['phase']}, capture={event['captureMs']:.3f}ms")
        for frame in event['stack']:
            name = frame.get('symbol') or f"{frame.get('image', '?')} + {frame['offset']}"
            if frame.get('uuid', '').lower() in uuids:
                matched = True
                key = (frame['base'], frame['pc'])
                if key not in cache:
                    cache[key] = subprocess.check_output(['atos', '-arch', 'arm64', '-o', str(args.binary), '-l', key[0], key[1]], text=True).strip()
                name = cache[key]
            print(f"  {frame['pc']} {name}")
if not matched:
    raise SystemExit('No stack frames matched this binary UUID. Keep the runtime binary from the installed build.')
