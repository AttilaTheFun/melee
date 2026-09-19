#!/usr/bin/env python3
"""Attempt to link the original game entry point with existing native backends.

Run audit.py and build_apple.py for the same SDK first. A successful object
compile is not a successful link; a successful link would not prove runtime
correctness. This tool never runs the resulting game or supplies fake symbols.
Aurora is not included: its extended GX ABI needs a separate full-game build.
"""
import argparse
import json
import re
from pathlib import Path
import subprocess
from sources import SOURCES

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', choices=['macosx', 'iphoneos'], default='macosx')
    args = parser.parse_args()
    audit_dir = ROOT / 'native/build' / ('audit-' + args.sdk)
    host_dir = ROOT / 'native/build' / args.sdk
    report = json.loads((audit_dir / 'report.json').read_text())
    if report['compiled'] != report['total']:
        parser.error('The compiler audit must pass before attempting the link')
    objects = [(ROOT / row['source'], audit_dir / Path(row['source']).with_suffix('.o'))
               for row in report['results']]
    audited_sources = {row['source'] for row in report['results']}
    objects += [(ROOT / source, host_dir / Path(source).with_suffix('.o'))
                for source in SOURCES if source not in audited_sources]
    for source, obj in objects:
        if not obj.is_file() or obj.stat().st_mtime_ns < source.stat().st_mtime_ns:
            parser.error(f'Missing or outdated object for {source.relative_to(ROOT)}; rerun the builds')
    output = ROOT / 'native/build' / ('link-' + args.sdk)
    output.mkdir(parents=True, exist_ok=True)
    binary = output / 'melee-game'
    binary.unlink(missing_ok=True)
    sdk = subprocess.check_output(['xcrun', '--sdk', args.sdk, '--show-sdk-path'], text=True).strip()
    command = ['xcrun', 'clang', '-target', report['target'], '-isysroot', sdk,
               '-Wl,-dead_strip', str(ROOT / 'native/aurora/game_audit_entry.c'),
               *[str(obj) for _, obj in objects], '-o', str(binary)]
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    (output / 'link.log').write_text(result.stdout + result.stderr)
    (output / 'report.json').write_text(json.dumps({
        'target': report['target'], 'object_count': len(objects),
        'linked': result.returncode == 0, 'returncode': result.returncode,
        'command': command, 'runtime_tested': False,
        'undefined_symbols': re.findall(r'^  "([^"]+)", referenced from:',
                                       result.stderr, re.MULTILINE),
    }, indent=2) + '\n')
    if result.returncode:
        binary.unlink(missing_ok=True)
    print(f"{report['target']}: {'linked' if result.returncode == 0 else 'link incomplete'} ({len(objects)} objects)")
    print(output / 'link.log')
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
