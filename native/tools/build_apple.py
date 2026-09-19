#!/usr/bin/env python3
"""Build the Apple platform test app. The Melee game runtime is not linked yet."""
import argparse
from pathlib import Path
import plistlib
import subprocess
from sources import SOURCES

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', choices=['macosx', 'iphoneos', 'iphonesimulator'], default='macosx')
    args = parser.parse_args()
    sdk = subprocess.check_output(['xcrun', '--sdk', args.sdk, '--show-sdk-path'], text=True).strip()
    target = {'macosx': 'arm64-apple-macos13', 'iphoneos': 'arm64-apple-ios16',
              'iphonesimulator': 'arm64-apple-ios16-simulator'}[args.sdk]
    build = ROOT / 'native/build' / args.sdk
    app = build / 'MeleeNative.app'
    contents = app / 'Contents' if args.sdk == 'macosx' else app
    binary_dir = contents / 'MacOS' if args.sdk == 'macosx' else app
    binary_dir.mkdir(parents=True, exist_ok=True)
    includes = ['-Inative/include', '-Iextern/dolphin/include', '-Isrc']
    objects = []
    for source in SOURCES:
        obj = build / Path(source).with_suffix('.o')
        obj.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['xcrun', '--sdk', args.sdk, 'clang', '-target', target, '-isysroot', sdk,
            '-std=gnu11', '-DMELEE_NATIVE', *includes, '-c', source, '-o', str(obj)], cwd=ROOT, check=True)
        objects.append(str(obj))
    subprocess.run(['xcrun', '--sdk', args.sdk, 'swiftc', '-target', target, '-sdk', sdk,
        '-module-cache-path', str(build / 'module-cache'), '-swift-version', '5',
        '-Xlinker', '-dead_strip',
        '-parse-as-library', '-import-objc-header', 'native/apple/Bridge.h',
        '-Xcc', '-DMELEE_NATIVE', *includes, 'native/apple/MeleeNative.swift', 'native/apple/MetalGeometryRenderer.swift', 'native/apple/AudioOutput.swift', *objects,
        '-o', str(binary_dir / 'MeleeNative')], cwd=ROOT, check=True)
    info = {'CFBundleExecutable': 'MeleeNative', 'CFBundleIdentifier': 'dev.melee.native',
        'CFBundleName': 'Melee Native', 'CFBundlePackageType': 'APPL',
        'CFBundleVersion': '1', 'CFBundleShortVersionString': '0.1',
        'NSHumanReadableCopyright': 'Native platform port development build'}
    if args.sdk == 'macosx':
        info['LSMinimumSystemVersion'] = '13.0'
        info['NSHighResolutionCapable'] = True
    else:
        info.update({'MinimumOSVersion': '16.0', 'UIDeviceFamily': [1, 2],
            'CFBundleSupportedPlatforms': ['iPhoneOS' if args.sdk == 'iphoneos' else 'iPhoneSimulator'],
            'UILaunchScreen': {}, 'UISupportedInterfaceOrientations':
            ['UIInterfaceOrientationLandscapeLeft', 'UIInterfaceOrientationLandscapeRight']})
    (contents / 'Info.plist').write_bytes(plistlib.dumps(info))
    if args.sdk != 'iphoneos':
        subprocess.run(['codesign', '--force', '--sign', '-', str(app)], check=True)
    print(app)
    if args.sdk == 'iphoneos':
        print('Unsigned device build: development signing and provisioning are required before installation.')


if __name__ == '__main__':
    main()
