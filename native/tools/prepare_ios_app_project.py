#!/usr/bin/env python3
"""Prepare the native game Xcode project for development signing on an iPhone/iPad.

Build the iphoneos runtime app first. Open the generated project, choose your
development team and connected device, then Run. No disc image is bundled.
"""
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
build = ROOT/'native/build/ios-device-project'
framework = ROOT/'native/build/iphoneos-game/MeleeNative.app/Frameworks/MeleeRuntime.framework'
if not (framework/'MeleeRuntime').exists():
    raise SystemExit('Run build_ios_runtime_app.py --sdk iphoneos first.')
build.mkdir(parents=True, exist_ok=True)
sources = ['MeleeNative.swift', 'GameView.swift', 'DirectMetalSurface.swift', 'MetalGeometryRenderer.swift',
           'MetalFXRenderer.swift', 'AudioOutput.swift']
spec = {
    'name': 'MeleeNative',
    'options': {'deploymentTarget': {'iOS': '17.0'}},
    'targets': {'MeleeNative': {
        'type': 'application', 'platform': 'iOS',
        'sources': [str(ROOT/'native/apple'/name) for name in sources],
        'dependencies': [{'framework': str(framework), 'embed': True}],
        'settings': {'base': {
            'PRODUCT_BUNDLE_IDENTIFIER': 'dev.melee.native.game',
            'SWIFT_VERSION': '5.0', 'TARGETED_DEVICE_FAMILY': '1,2',
            'SUPPORTED_PLATFORMS': 'iphoneos', 'CODE_SIGN_STYLE': 'Automatic',
            'SWIFT_ACTIVE_COMPILATION_CONDITIONS': 'MELEE_GAME_RUNTIME',
            'SWIFT_OBJC_BRIDGING_HEADER': str(ROOT/'native/apple/Bridge.h'),
            'HEADER_SEARCH_PATHS': [str(ROOT/p) for p in ['native/include', 'extern/dolphin/include', 'src']],
            'OTHER_SWIFT_FLAGS': ['$(inherited)', '-Xcc', '-DMELEE_NATIVE'],
            'DEAD_CODE_STRIPPING': 'YES',
        }},
        'info': {'path': str(build/'Info.plist'), 'properties': {
            'CFBundleDisplayName': 'Melee Native', 'UILaunchScreen': {},
            'UISupportedInterfaceOrientations': [
                'UIInterfaceOrientationLandscapeLeft', 'UIInterfaceOrientationLandscapeRight'],
        }},
    }},
    'schemes': {'MeleeNative': {'build': {'targets': {'MeleeNative': 'all'}}}},
}
path = build/'project.json'
path.write_text(json.dumps(spec, indent=2)+'\n')
subprocess.run(['xcodegen', '--spec', str(path), '--project', str(build)], check=True)
print(build/'MeleeNative.xcodeproj')
