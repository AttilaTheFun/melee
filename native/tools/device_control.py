#!/usr/bin/env python3
"""Send bounded synthetic pad input to an explicitly opted-in physical test app.

Launch Melee with MELEE_DEVICE_CONTROL=1 and MELEE_DEVICE_DIAGNOSTICS=1.
This verifies the device runtime, not physical touch delivery. All transfers
stay in Melee's own app container; no extra app or local server is installed.
"""
import argparse
import json
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--device', required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--button', choices=['A', 'B', 'X', 'Y', 'Z', 'L', 'R', 'Start'])
parser.add_argument('--x', type=float, default=0)
parser.add_argument('--y', type=float, default=0)
parser.add_argument('--duration', type=float, default=0.15)
parser.add_argument('--metalfx', type=int, choices=[0, 2, 3])
args = parser.parse_args()
if not -1 <= args.x <= 1 or not -1 <= args.y <= 1 or not 0 <= args.duration <= 3:
    parser.error('Axes must be within -1...1 and duration within 0...3 seconds')
buttons = dict(A=0x100, B=0x200, X=0x400, Y=0x800, Z=0x10, L=0x40, R=0x20, Start=0x1000)
sequence = time.time_ns()
args.output.mkdir(parents=True, exist_ok=True)
command = args.output/'sent-command.json'
command.write_text(json.dumps(dict(sequence=sequence, buttons=buttons.get(args.button, 0),
                                  x=args.x, y=args.y, duration=args.duration, metalFXScale=args.metalfx)))
base = ['xcrun', 'devicectl', 'device', 'copy']
domain = ['--device', args.device, '--domain-type', 'appDataContainer',
          '--domain-identifier', 'dev.melee.native.game']
subprocess.run(base+['to', *domain, '--source', str(command),
                     '--destination', 'Documents/Diagnostics/command.json'], check=True, timeout=30)
time.sleep(args.duration+2.5)
for attempt in range(6):
    subprocess.run(base+['from', *domain, '--source', 'Documents/Diagnostics',
                         '--destination', str(args.output/'received')], check=True, timeout=30)
    received = args.output/'received'
    ack_path = received/'input-ack.json'
    if not ack_path.exists():
        raise SystemExit('Fresh-frame diagnostics require an updated test app.')
    ack = json.loads(ack_path.read_text())
    state = json.loads((received/'state.json').read_text())
    scaled_ready = True
    if args.metalfx in (2, 3):
        try:
            scaled = json.loads((received/'metalfx.json').read_text())
            scaled_ready = (scaled['width'] == 640 * args.metalfx and
                            scaled['height'] == 480 * args.metalfx and
                            scaled['uptime'] >= ack['uptime'])
        except (FileNotFoundError, KeyError, json.JSONDecodeError):
            scaled_ready = False
    if (ack['sequence'] == sequence and state.get('uptime', 0) >= ack['uptime'] + args.duration
            and scaled_ready):
        print(json.dumps(state, sort_keys=True))
        print(received/'frame.png')
        break
    if attempt == 5:
        raise SystemExit('No fresh frame after the input; inspect the device log for a crash or stalled runtime.')
    time.sleep(2)
