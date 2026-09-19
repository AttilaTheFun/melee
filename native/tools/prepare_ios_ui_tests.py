#!/usr/bin/env python3
"""Generate an isolated Xcode UI-test project for the real simulator game app."""
from pathlib import Path
import argparse
import os
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--session-dir', type=Path, help='Enable the opt-in interactive simulator control test')
args = parser.parse_args()
build = ROOT/'native/build/ios-ui-tests'
build.mkdir(parents=True, exist_ok=True)
framework = ROOT/'native/build/iphonesimulator-game/MeleeNative.app/Frameworks/MeleeRuntime.framework'
if not (framework/'MeleeRuntime').exists():
    raise SystemExit('Build the simulator game runtime/app first.')
settings = {
    'SWIFT_VERSION': '5.0', 'IPHONEOS_DEPLOYMENT_TARGET': '17.0',
    'SUPPORTED_PLATFORMS': 'iphonesimulator', 'TARGETED_DEVICE_FAMILY': '1,2',
    'CODE_SIGN_STYLE': 'Automatic',
}
spec = {
    'name': 'MeleeUITests', 'options': {'deploymentTarget': {'iOS': '17.0'}},
    'settings': {'base': settings},
    'targets': {
        'MeleeUITestGame': {
            'type': 'application', 'platform': 'iOS',
            'sources': [str(ROOT/'native/apple'/name) for name in
                        ['MeleeNative.swift', 'GameView.swift', 'DirectMetalSurface.swift', 'MetalGeometryRenderer.swift', 'MetalFXRenderer.swift', 'AudioOutput.swift']],
            'dependencies': [{'framework': str(framework), 'embed': True}],
            'settings': {'base': {
                'PRODUCT_BUNDLE_IDENTIFIER': 'dev.melee.native.uitestgame',
                'SWIFT_ACTIVE_COMPILATION_CONDITIONS': 'MELEE_GAME_RUNTIME',
                'SWIFT_OBJC_BRIDGING_HEADER': str(ROOT/'native/apple/Bridge.h'),
                'HEADER_SEARCH_PATHS': [str(ROOT/p) for p in ['native/include','extern/dolphin/include','src']],
                'OTHER_SWIFT_FLAGS': ['$(inherited)', '-Xcc', '-DMELEE_NATIVE'],
                'DEAD_CODE_STRIPPING': 'YES',
            }},
            'info': {'path': str(build/'AppInfo.plist'), 'properties': {
                'CFBundleDisplayName': 'Melee UI Test', 'UILaunchScreen': {},
                'UISupportedInterfaceOrientations': [
                    'UIInterfaceOrientationLandscapeLeft', 'UIInterfaceOrientationLandscapeRight'],
            }},
        },
        'MeleeGameUITests': {
            'type': 'bundle.ui-testing', 'platform': 'iOS',
            'sources': [str(ROOT/'native/tests/apple/GameUITests.swift')],
            'dependencies': [{'target': 'MeleeUITestGame'}],
            'settings': {'base': {
                'PRODUCT_BUNDLE_IDENTIFIER': 'dev.melee.native.uitests',
                'GENERATE_INFOPLIST_FILE': 'YES', 'TEST_TARGET_NAME': 'MeleeUITestGame',
            }},
        },
    },
    'schemes': {'MeleeUITests': {
        'build': {'targets': {'MeleeUITestGame': 'all', 'MeleeGameUITests': ['test']}},
        'test': {'targets': [{'name': 'MeleeGameUITests', 'parallelizable': False}],
                 'environmentVariables': {'MELEE_UI_TEST_DISC': str(ROOT/'native/build/simulator-test.ciso')}},
    }},
}
if os.environ.get('MELEE_UI_TEST_APPEARANCE'):
    spec['schemes']['MeleeUITests']['test']['environmentVariables']['MELEE_UI_TEST_APPEARANCE'] = os.environ['MELEE_UI_TEST_APPEARANCE']
path = build/'project.json'
if args.session_dir:
    directory = args.session_dir.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    spec['schemes']['MeleeUITests']['test']['environmentVariables']['MELEE_UI_SESSION_DIR'] = str(directory)
path.write_text(json.dumps(spec, indent=2)+'\n')
subprocess.run(['xcodegen', '--spec', str(path), '--project', str(build)], check=True)
print(build/'MeleeUITests.xcodeproj')
