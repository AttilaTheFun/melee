#!/usr/bin/env python3
"""Stage an allowlisted site: never publish discs, saves, logs or test artifacts."""
import hashlib
import json
from pathlib import Path
import shutil

WEB = Path(__file__).resolve().parent
BUILD = WEB.parent / 'build' / 'wasm-renderer'
SITE = WEB.parent / 'build' / 'wasm-site'

def main():
    files = {name: BUILD / name for name in ('melee_browser.js', 'melee_browser.wasm')}
    files.update({name: WEB / 'game' / name for name in ('index.html', 'audio-worklet.js')})
    files.update({'netplay/' + name: WEB / 'netplay' / name for name in
                  ('rooms.mjs', 'transport.mjs', 'session.mjs', 'inputs.mjs')})
    for path in files.values():
        if not path.is_file():
            raise SystemExit(f'Missing build input: {path}')
    # Include JS glue and protocol code as well as Wasm in compatibility identity.
    digest = hashlib.sha256()
    for name, path in sorted(files.items()):
        content = path.read_bytes()
        digest.update(name.encode() + b'\0' + len(content).to_bytes(8, 'little') + content)
    staging = SITE.with_name(SITE.name + '.staging')
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    for name, path in files.items():
        target = staging / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    (staging / 'build.json').write_text(json.dumps({'build': digest.hexdigest()}) + '\n')
    if SITE.exists():
        shutil.rmtree(SITE)
    staging.rename(SITE)
    print(f'Staged {len(files) + 1} public files in {SITE}')

if __name__ == '__main__':
    main()
