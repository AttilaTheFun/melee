#!/usr/bin/env python3
"""Send a touch command to an already-running opt-in XCTest simulator session."""
import argparse
import json
from pathlib import Path
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory', type=Path)
parser.add_argument('action', choices=['snapshot', 'press', 'stick', 'quit'])
parser.add_argument('--button', choices=['A', 'B', 'X', 'Y', 'Z', 'L', 'R', 'Start'])
parser.add_argument('--x', type=float, default=0)
parser.add_argument('--y', type=float, default=0)
parser.add_argument('--duration', type=float, default=0.15)
args = parser.parse_args()
if not -1 <= args.x <= 1 or not -1 <= args.y <= 1 or not 0 <= args.duration <= 5:
    parser.error('Axes must be within -1...1 and duration within 0...5 seconds')
if args.action == 'press' and not args.button:
    parser.error('press requires --button')
state_path = args.directory/'state.json'
state = json.loads(state_path.read_text())
command_path = args.directory/'command.json'
if command_path.exists():
    pending = json.loads(command_path.read_text())
    if pending['sequence'] > state['sequence']:
        raise SystemExit('A command is still pending; wait for acknowledgement before sending another.')
sequence = state['sequence']+1
command = dict(sequence=sequence, action=args.action, button=args.button,
               x=args.x, y=args.y, duration=args.duration)
temporary = args.directory/'command.tmp'
temporary.write_text(json.dumps(command))
temporary.replace(args.directory/'command.json')
deadline = time.monotonic()+30
while time.monotonic() < deadline:
    state = json.loads(state_path.read_text())
    if state['sequence'] >= sequence:
        print(json.dumps(state))
        print(args.directory/f'frame-{sequence}.png')
        break
    time.sleep(0.1)
else:
    raise SystemExit('No acknowledgement: inspect the existing XCTest process before sending another command')
