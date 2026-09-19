#!/usr/bin/env python3
"""Compile the original game/engine translation units for Apple ARM64.

This is a compiler audit, not a linked or playable game. No generated stubs
or suppressed compiler errors are used to inflate the success count.
"""
import argparse
import concurrent.futures
import json
from pathlib import Path
import subprocess
from prepare_fonts import DEFAULT_DOL, prepare

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', choices=['macosx', 'iphoneos', 'iphonesimulator'], default='macosx')
    parser.add_argument('--jobs', type=int, default=8)
    args = parser.parse_args()
    sdk = subprocess.check_output(['xcrun', '--sdk', args.sdk, '--show-sdk-path'], text=True).strip()
    target = {'macosx': 'arm64-apple-macos13', 'iphoneos': 'arm64-apple-ios16',
              'iphonesimulator': 'arm64-apple-ios16-simulator'}[args.sdk]
    output = ROOT / 'native/build' / ('audit-' + args.sdk)
    output.mkdir(parents=True, exist_ok=True)
    generated = prepare() if DEFAULT_DOL.is_file() else ROOT / "native/build/generated"
    sources = sorted(p for folder in ['src/melee', 'src/sysdolphin']
                     for p in (ROOT / folder).rglob('*.c'))

    def compile_one(source):
        relative = source.relative_to(ROOT)
        obj = output / relative.with_suffix('.o')
        obj.parent.mkdir(parents=True, exist_ok=True)
        command = ['xcrun', 'clang', '-target', target, '-isysroot', sdk,
                   '-std=gnu11', '-DMELEE_NATIVE', '-fno-common', '-ferror-limit=5',
                   '-Isrc', '-Inative/include', '-Iextern/dolphin/include', '-Iextern/dolphin/src', '-I' + str(generated),
                   '-c', str(relative), '-o', str(obj)]
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        obj.with_suffix('.log').write_text(result.stderr)
        if result.returncode and obj.exists():
            obj.unlink()
        return {'source': str(relative), 'compiled': result.returncode == 0,
                'errors': [line for line in result.stderr.splitlines() if 'error:' in line],
                'command': command}

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(compile_one, sources))
    passed = sum(r['compiled'] for r in results)
    report = {'target': target, 'compiled': passed, 'total': len(results), 'results': results}
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{target}: {passed}/{len(results)} translation units compiled. This does not test linking or runtime.')
    print(output / 'report.json')
    return 0 if passed == len(results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
