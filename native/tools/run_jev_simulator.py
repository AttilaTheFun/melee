#!/usr/bin/env python3
"""Build and run Jev shader-coverage testing on the dedicated Melee simulator."""
import argparse
import json
import tempfile
from jev_maze_route import extract
import os
from pathlib import Path
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]
TOOLS=ROOT/'native/tools'
BUNDLE='dev.melee.native.game'


def run(*args,**kwargs):
    return subprocess.run(list(map(str,args)),check=True,**kwargs)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--disc',type=Path,required=True)
    p.add_argument('--run-dir',type=Path,required=True)
    p.add_argument('--max-cost',type=float,default=2)
    p.add_argument('--max-calls',type=int,default=10000)
    p.add_argument('--max-seconds',type=float,default=7200)
    p.add_argument('--real-time',action='store_true',help='Do not pause between controller actions')
    p.add_argument('--skip-build',action='store_true')
    a=p.parse_args()
    if not a.disc.is_file():p.error('Disc image not found')
    if not (os.environ.get('OPEN_ROUTER_API_KEY') or os.environ.get('OPENROUTER_API_KEY')):p.error('OpenRouter key is not set in the environment')
    root=a.run_dir.resolve();root.mkdir(parents=True,exist_ok=True)
    completion=root/'completion.json'
    if completion.exists():completion.rename(root/f'completion-previous-{time.time_ns()}.json')
    run('make','-C',ROOT/'native','build/inspect-disc')
    with tempfile.NamedTemporaryFile(suffix='.dat') as archive:
        run(ROOT/'native/build/inspect-disc',a.disc.resolve(),'GrNSr.dat',stdout=archive)
        archive.flush()
        geometry=extract(Path(archive.name),geometry=True)
        (root/'maze-markers.json').write_text(json.dumps(geometry['markers'],indent=2)+'\n')
        (root/'maze-geometry.json').write_text(json.dumps(geometry)+'\n')
    (root/'maze-progress.json').unlink(missing_ok=True)
    if not a.skip_build:run('python3',TOOLS/'build_ios_runtime_app.py','--sdk','iphonesimulator')
    device=subprocess.check_output(['python3',str(TOOLS/'melee_simulator.py'),'--boot'],text=True).strip()
    try:
        run('xcrun','simctl','bootstatus',device,'-b')
        run('xcrun','simctl','install',device,ROOT/'native/build/iphonesimulator-game/MeleeNative.app')
        env=os.environ.copy()
        env.update(SIMCTL_CHILD_MELEE_AGENT_DIR=str(root),SIMCTL_CHILD_MELEE_DISC_IMAGE=str(a.disc.resolve()),SIMCTL_CHILD_MELEE_TEST_SILENT='1')
        env.pop('SIMCTL_CHILD_MELEE_AGENT_STEP',None)
        if not a.real_time:env['SIMCTL_CHILD_MELEE_AGENT_STEP']='1'
        started=time.time()
        run('xcrun','simctl','launch','--terminate-running-process',device,BUNDLE,env=env)
        started=time.time()
        while not (root/'state.json').exists() or (root/'state.json').stat().st_mtime<started:
            if time.time()-started>180:raise TimeoutError('No simulator observations after shader preparation')
            time.sleep(.5)
        run('python3',TOOLS/'jev_play.py',root,'--device',device,'--max-cost',a.max_cost,'--max-calls',a.max_calls,'--max-seconds',a.max_seconds)
    finally:
        subprocess.run(['xcrun','simctl','terminate',device,BUNDLE],check=False)
        try:run('python3',TOOLS/'collect_simulator_shaders.py','--device',device,'--output',root/f'catalog-{time.time_ns()}.db')
        finally:run('python3',TOOLS/'melee_simulator.py','--shutdown')

if __name__=='__main__':main()
