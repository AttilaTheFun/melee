#!/usr/bin/env python3
"""Snapshot a simulator app's learned shader catalog, including its live WAL.

No game images or saves are copied. Pass the snapshot to merge_shader_catalogs.py
with the existing seed; review the merged output before replacing the seed.
"""
import argparse
from pathlib import Path
import sqlite3
import subprocess


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--device',required=True)
    p.add_argument('--bundle',default='dev.melee.native.game')
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists():p.error('Output already exists; choose a new snapshot path.')
    container=Path(subprocess.check_output(['xcrun','simctl','get_app_container',a.device,a.bundle,'data'],text=True).strip())
    source=container/'Library/Caches/MeleeNative/GPU/pipeline_cache.db'
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with sqlite3.connect(source.as_uri()+'?mode=ro',uri=True) as src,sqlite3.connect(a.output) as dst:
        src.backup(dst,pages=256)
        if dst.execute('pragma integrity_check').fetchone()!=('ok',):raise RuntimeError('Snapshot integrity check failed')
        count=dst.execute('select count(*) from pipeline_cache').fetchone()[0]
    print(f'{count} shader configurations saved to {a.output}')

if __name__=='__main__':main()
