#!/usr/bin/env python3
"""Run the browser worker smoke test in the dedicated iOS Simulator, not host UI.

A temporary loopback server is closed and the probe app is terminated on exit.
The simulator remains booted. No real saves are used. GPU absence is recorded;
--require-gpu makes it fail the run. Desktop Safari still needs its own check.
"""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import threading
import time

NATIVE=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('disc',type=Path)
parser.add_argument('--require-gpu',action='store_true')
args=parser.parse_args()
OUT=NATIVE/'build/webkit-test'
OUT.mkdir(parents=True,exist_ok=True)
APP=OUT/'WebKitProbe.app'
APP.mkdir(exist_ok=True)
BUNDLE='dev.melee.webgpu-probe'
def run(*cmd,**kwargs):
    return subprocess.check_output([str(x) for x in cmd],text=True,**kwargs).strip()

subprocess.run(['python3',str(NATIVE/'web/make-test-fixture.py'),str(args.disc.resolve()),str(OUT/'fixture.iso')],check=True)
for name in ['index.html','app.js','renderer.js','worker.js','scene-data.js','melee.mjs','melee.wasm']:
    shutil.copy2(NATIVE/'build/web'/name,OUT/name)
shutil.copy2(NATIVE/'web/smoke.html',OUT/'smoke.html')
info={'CFBundleIdentifier':BUNDLE,'CFBundleName':'Melee WebGPU Probe','CFBundleExecutable':'WebKitProbe','CFBundlePackageType':'APPL','CFBundleVersion':'1','CFBundleShortVersionString':'1','LSRequiresIPhoneOS':True,'UILaunchScreen':{},'UIDeviceFamily':[1,2],'NSAppTransportSecurity':{'NSAllowsLocalNetworking':True},'UIApplicationSceneManifest':{'UIApplicationSupportsMultipleScenes':False,'UISceneConfigurations':{}}}
with (APP/'Info.plist').open('wb') as file:plistlib.dump(info,file)
run('xcrun','--sdk','iphonesimulator','swiftc','-target','arm64-apple-ios17.0-simulator','-sdk',run('xcrun','--sdk','iphonesimulator','--show-sdk-path'),'-parse-as-library',NATIVE/'web/WebKitProbe.swift','-o',APP/'WebKitProbe')
run('codesign','--force','--sign','-',APP)
uuid=run('python3',NATIVE/'tools/melee_simulator.py','--boot')
run('xcrun','simctl','bootstatus',uuid,'-b')
run('xcrun','simctl','install',uuid,APP)
container=Path(run('xcrun','simctl','get_app_container',uuid,BUNDLE,'data'))
result=container/'Documents/result.json'
result.unlink(missing_ok=True)
class Handler(SimpleHTTPRequestHandler):
    def log_message(self,*args):pass
server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(OUT)))
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
env=dict(os.environ,SIMCTL_CHILD_MELEE_WEB_URL=f'http://127.0.0.1:{server.server_port}/smoke.html')
try:
    run('xcrun','simctl','launch',uuid,BUNDLE,env=env)
    deadline=time.monotonic()+60
    while time.monotonic()<deadline:
        try:report=json.loads(result.read_text())
        except (FileNotFoundError,json.JSONDecodeError):report={}
        if report.get('workerPassed') or report.get('error'):break
        time.sleep(.2)
    else:raise RuntimeError('WebKit smoke test timed out')
    (OUT/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    if not report.get('workerPassed') or (args.require_gpu and not report.get('gpuPassed')):
        raise SystemExit(1)
finally:
    subprocess.run(['xcrun','simctl','terminate',uuid,BUNDLE],capture_output=True)
    server.shutdown();server.server_close();thread.join()
