#!/usr/bin/env python3
"""Package the actual macOS game runtime with the Apple controls UI."""
import argparse
from pathlib import Path
import plistlib
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def command(*args):
    return subprocess.check_output(args, text=True).strip()

def dependencies(path):
    return [line.strip().split(' (compatibility')[0] for line in command('otool', '-L', str(path)).splitlines()[1:]]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path)
    args = parser.parse_args()
    if args.runtime is None:
        subprocess.run(['cmake', '--build', str(ROOT/'native/build/aurora-integration'), '--target', 'melee_game_runtime', '-j8'], check=True)
    runtime = (args.runtime or ROOT/'native/build/aurora-integration/libmelee_game_runtime.dylib').resolve(strict=True)
    build = ROOT/'native/build/macosx-game'
    contents = build/'MeleeNative.app/Contents'
    frameworks = contents/'Frameworks'
    binary_dir = contents/'MacOS'
    frameworks.mkdir(parents=True, exist_ok=True)
    binary_dir.mkdir(parents=True, exist_ok=True)
    clang_resources = Path(command('xcrun', 'clang', '--print-resource-dir'))/'lib/darwin'
    queued, copied = [runtime], {}
    while queued:
        source = queued.pop().resolve(strict=True)
        if source.name in copied:
            if copied[source.name] != source:
                raise RuntimeError(f'Duplicate library name: {source.name}')
            continue
        copied[source.name] = source
        destination = frameworks/source.name
        shutil.copy2(source, destination)
        destination.chmod(destination.stat().st_mode | 0o200)
        replacements = []
        for dep in dependencies(source):
            if dep.startswith(('/System/', '/usr/lib/')) or dep.endswith('/'+source.name):
                continue
            if dep.startswith('@rpath/'):
                candidates = [source.parent/dep[7:], clang_resources/dep[7:]]
                resolved = next((p for p in candidates if p.exists()), None)
                if resolved is None: raise RuntimeError(f'Cannot resolve {dep} from {source}')
            elif dep.startswith('@loader_path/'):
                resolved = source.parent/dep[len('@loader_path/'):]
            else:
                resolved = Path(dep)
            resolved = resolved.resolve(strict=True)
            queued.append(resolved)
            replacements.extend(['-change', dep, '@loader_path/'+resolved.name])
        subprocess.run(['install_name_tool', '-id', '@rpath/'+source.name, *replacements, str(destination)], check=True)
    minimum = next(line.split()[1] for line in command('xcrun','vtool','-show-build',str(runtime)).splitlines() if line.strip().startswith('minos '))
    sdk = command('xcrun','--sdk','macosx','--show-sdk-path')
    includes = ['-Inative/include','-Iextern/dolphin/include','-Isrc']
    asan = 'libclang_rt.asan_osx_dynamic.dylib' in copied
    flags = ['-sanitize=address'] if asan else []
    subprocess.run(['xcrun','swiftc','-target',f'arm64-apple-macos{minimum}','-sdk',sdk,
        '-module-cache-path',str(build/'module-cache'),'-swift-version','5',
        '-parse-as-library','-DMELEE_GAME_RUNTIME',*flags,'-import-objc-header','native/apple/Bridge.h',
        '-Xcc','-DMELEE_NATIVE',*includes,'native/apple/MeleeNative.swift','native/apple/GameView.swift','native/apple/DirectMetalSurface.swift',
        'native/apple/MetalGeometryRenderer.swift','native/apple/MetalFXRenderer.swift','native/apple/AudioOutput.swift',
        '-L'+str(frameworks),'-lmelee_game_runtime','-Xlinker','-rpath','-Xlinker','@executable_path/../Frameworks',
        '-Xlinker','-dead_strip','-o',str(binary_dir/'MeleeNative')],cwd=ROOT,check=True)
    info = {'CFBundleExecutable':'MeleeNative','CFBundleIdentifier':'dev.melee.native.game',
        'CFBundleName':'Melee Native','CFBundlePackageType':'APPL','CFBundleVersion':'1',
        'CFBundleShortVersionString':'0.2','LSMinimumSystemVersion':minimum,'NSHighResolutionCapable':True}
    (contents/'Info.plist').write_bytes(plistlib.dumps(info))
    for library in frameworks.glob('*.dylib'):
        subprocess.run(['codesign','--force','--sign','-',str(library)],check=True)
    subprocess.run(['codesign','--force','--sign','-',str(contents.parent)],check=True)
    print(contents.parent)

if __name__ == '__main__': main()
