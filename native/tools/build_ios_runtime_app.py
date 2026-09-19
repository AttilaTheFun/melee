#!/usr/bin/env python3
"""Package the actual iOS game runtime with the Apple touch/controller UI."""
import argparse
from pathlib import Path
import plistlib
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def output(*args):
    return subprocess.check_output(args, text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', choices=['iphoneos', 'iphonesimulator'], default='iphoneos')
    parser.add_argument('--runtime', type=Path)
    args = parser.parse_args()
    if args.runtime is None:
        subprocess.run(['cmake', '--build', str(ROOT/f'native/build/aurora-{args.sdk}'), '--target', 'melee_game_runtime', '-j8'], check=True)
    runtime = (args.runtime or ROOT/f'native/build/aurora-{args.sdk}/libmelee_game_runtime.dylib').resolve(strict=True)
    version = output('xcrun', 'vtool', '-show-build', str(runtime)).splitlines()
    platform = next(line.split()[1] for line in version if line.strip().startswith('platform '))
    expected_platform = 'IOS' if args.sdk == 'iphoneos' else 'IOSSIMULATOR'
    if platform != expected_platform:
        raise SystemExit(f'Expected {expected_platform} runtime, got {platform}')
    minimum = next(line.split()[1] for line in version if line.strip().startswith('minos '))
    dependencies = output('otool', '-L', str(runtime)).splitlines()[2:]
    for line in dependencies:
        dependency = line.strip().split(' (compatibility')[0]
        if not dependency.startswith(('/System/', '/usr/lib/')):
            raise SystemExit(f'Runtime dependency must be statically linked: {dependency}')

    build = ROOT/f'native/build/{args.sdk}-game'
    app = build/'MeleeNative.app'
    framework = app/'Frameworks/MeleeRuntime.framework'
    framework.mkdir(parents=True, exist_ok=True)
    binary = framework/'MeleeRuntime'
    shutil.copy2(runtime, binary)
    binary.chmod(binary.stat().st_mode | 0o200)
    subprocess.run(['install_name_tool', '-id',
                    '@rpath/MeleeRuntime.framework/MeleeRuntime', str(binary)], check=True)
    framework_info = {
        'CFBundleExecutable': 'MeleeRuntime', 'CFBundleIdentifier': 'dev.melee.native.runtime',
        'CFBundleName': 'MeleeRuntime', 'CFBundlePackageType': 'FMWK',
        'CFBundleVersion': '1', 'CFBundleShortVersionString': '0.2',
        'MinimumOSVersion': minimum,
        'CFBundleSupportedPlatforms': ['iPhoneOS' if args.sdk == 'iphoneos' else 'iPhoneSimulator'],
    }
    (framework/'Info.plist').write_bytes(plistlib.dumps(framework_info))
    sdk = output('xcrun', '--sdk', args.sdk, '--show-sdk-path')
    target = f'arm64-apple-ios{minimum}' + ('-simulator' if args.sdk == 'iphonesimulator' else '')
    subprocess.run([
        'xcrun', '--sdk', args.sdk, 'swiftc', '-target', target,
        '-sdk', sdk, '-module-cache-path', str(build/'module-cache'), '-swift-version', '5',
        '-parse-as-library', '-DMELEE_GAME_RUNTIME', '-import-objc-header', 'native/apple/Bridge.h',
        '-Xcc', '-DMELEE_NATIVE', '-Inative/include', '-Iextern/dolphin/include', '-Isrc',
        'native/apple/MeleeNative.swift', 'native/apple/GameView.swift','native/apple/DirectMetalSurface.swift',
        'native/apple/MetalGeometryRenderer.swift','native/apple/MetalFXRenderer.swift', 'native/apple/AudioOutput.swift',
        '-F'+str(app/'Frameworks'), '-framework', 'MeleeRuntime',
        '-Xlinker', '-rpath', '-Xlinker', '@executable_path/Frameworks',
        '-Xlinker', '-dead_strip', '-o', str(app/'MeleeNative'),
    ], cwd=ROOT, check=True)
    info = {
        'CFBundleExecutable': 'MeleeNative', 'CFBundleIdentifier': 'dev.melee.native.game',
        'CFBundleName': 'Melee Native', 'CFBundlePackageType': 'APPL',
        'CFBundleVersion': '1', 'CFBundleShortVersionString': '0.2',
        'MinimumOSVersion': minimum, 'UIDeviceFamily': [1, 2],
        'CFBundleSupportedPlatforms': framework_info['CFBundleSupportedPlatforms'], 'UILaunchScreen': {},
        'UISupportedInterfaceOrientations': [
            'UIInterfaceOrientationLandscapeLeft', 'UIInterfaceOrientationLandscapeRight'],
    }
    (app/'Info.plist').write_bytes(plistlib.dumps(info))
    if args.sdk == 'iphonesimulator':
        subprocess.run(['codesign', '--force', '--sign', '-', str(framework)], check=True)
        subprocess.run(['codesign', '--force', '--sign', '-', str(app)], check=True)
    print(app)
    if args.sdk == 'iphoneos':
        print('Unsigned device build: development signing and provisioning are required before installation.')


if __name__ == '__main__':
    main()
