#!/usr/bin/env python3
"""Sequential native gameplay checkpoints and fresh-process save regression.

Build melee_game_startup and melee_runtime_probe first. Do not run alongside
other GPU/game/Simulator tests. Checkpoints use controlled input/state; this
does not certify normal human play through every mode. Saves are disposable.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import subprocess
import signal
import sys
import time

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc', type=Path, default=ROOT/'native/build/simulator-test.ciso')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--campaigns', action='store_true', help='Also run controlled Classic and All-Star campaigns')
    args = parser.parse_args()
    disc = args.disc.resolve(strict=True)
    probe = (ROOT/'native/build/aurora-integration/melee_game_startup').resolve(strict=True)
    output = args.output or ROOT/'native/build'/('playable-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    # Do not inherit old campaign, save, or debug fixtures from the shell.
    environment = {k:v for k,v in os.environ.items() if not k.startswith('MELEE_')}
    environment['ASAN_OPTIONS'] = 'detect_leaks=0'
    steps = [(name, [str(probe), str(disc), str(output/'gpu-cache'), str(output/(name+'.png')), '--'+name])
             for name in ['classic-match', 'classic-round', 'classic-gameover', 'rematch']]
    steps.append(('save-reload', [sys.executable, str(ROOT/'native/tools/test_runtime_save.py'), '--disc', str(disc), '--output', str(output/'save-reload')]))
    if args.campaigns:
        steps.extend([
            ('classic-campaign', [str(probe),str(disc),str(output/'gpu-cache'),str(output/'classic-campaign.png'),'--classic-campaign']),
            ('allstar-campaign', [sys.executable,str(ROOT/'native/tools/test_allstar.py'),'--disc',str(disc)])])
    report = {'disc':str(disc), 'scope':'controlled checkpoints, not full manual gameplay', 'steps':[]}
    for name, command in steps:
        print(f'Running {name}: {output/(name+".log")}', flush=True)
        started = time.monotonic()
        with (output/(name+'.log')).open('w') as log:
            process = subprocess.Popen(command,cwd=ROOT,env=environment,stdout=log,
                                       stderr=subprocess.STDOUT,start_new_session=True)
            try:
                code = process.wait(timeout=2700)
            except (subprocess.TimeoutExpired, KeyboardInterrupt):
                # Wrappers launch native children. Stop the whole group before
                # returning so an orphan GPU test cannot overlap the next run.
                os.killpg(process.pid, signal.SIGTERM)
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
                code = 124
        report['steps'].append({'name':name,'command':command,'exitCode':code,'seconds':time.monotonic()-started})
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        if code:
            raise SystemExit(f'{name} failed ({code}); see its log. Remaining steps were not run.')
    print(f'All {len(steps)} gameplay/save checkpoints passed. Report: {output/"report.json"}')

if __name__ == '__main__':
    main()
