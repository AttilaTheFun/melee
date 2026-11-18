#!/usr/bin/env python3
"""Windowless runtime regression with a populated pipeline cache.

Copies only pipeline configurations into a new directory, leaving Dawn's disk
cache empty. Driver/OS caches are not reset. The runtime probe checks advancing,
nonblack frames and pause/resume. This does not test direct surface presentation.
Run sequentially with all other game/GPU tests. No existing saves are opened.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import signal
import sqlite3
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pipeline-cache', required=True, type=Path)
    parser.add_argument('--disc', type=Path, default=ROOT/'native/build/simulator-test.ciso')
    parser.add_argument('--probe', type=Path, default=ROOT/'native/build/aurora-integration/melee_runtime_probe')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--timeout', type=float, default=30)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    source = args.pipeline_cache.resolve(strict=True)
    probe = args.probe.resolve(strict=True)
    disc = args.disc.resolve(strict=True)
    output = (args.output or ROOT/'native/build'/
              ('cached-startup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))).resolve()
    output.mkdir(parents=True, exist_ok=False)
    cache = output/'gpu-cache'
    cache.mkdir()
    with sqlite3.connect(source.as_uri()+'?mode=ro', uri=True) as src:
        with sqlite3.connect(cache/'pipeline_cache.db') as dst:
            src.backup(dst)
            rows = dst.execute('SELECT count(*) FROM pipeline_cache').fetchone()[0]
    if not rows:
        raise SystemExit('The regression needs a populated pipeline cache.')
    environment = {k: v for k, v in os.environ.items() if not k.startswith('MELEE_')}
    environment['ASAN_OPTIONS'] = 'detect_leaks=0'
    command = [str(probe), str(disc), str(cache), str(output/'frame.png')]
    started = time.monotonic()
    with (output/'runtime.log').open('w') as log:
        process = subprocess.Popen(command, cwd=ROOT, env=environment, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = process.wait(timeout=args.timeout)
        except (subprocess.TimeoutExpired, KeyboardInterrupt):
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            code = 124
    report = {'cachedPipelines': rows, 'exitCode': code,
              'seconds': time.monotonic()-started, 'timeout': args.timeout,
              'command': command, 'scope': 'windowless cached startup and runtime pause/resume'}
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2), flush=True)
    raise SystemExit(code)


if __name__ == '__main__':
    main()
