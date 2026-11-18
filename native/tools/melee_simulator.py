#!/usr/bin/env python3
"""Select/create the Melee-owned simulator; never operate on a shared device.

Prints its UUID for xcodebuild destinations. Boot and shutdown are opt-in and
always target this exact named device. No erase/delete or 'booted' alias is used.
"""
import argparse
import json
import subprocess

NAME = 'Melee Dedicated iPhone 17 Pro'
TYPE = 'com.apple.CoreSimulator.SimDeviceType.iPhone-17-Pro'
RUNTIME = 'com.apple.CoreSimulator.SimRuntime.iOS-27-0'


def run(*args):
    return subprocess.check_output(['xcrun', 'simctl', *args], text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_mutually_exclusive_group()
    actions.add_argument('--boot', action='store_true')
    actions.add_argument('--shutdown', action='store_true')
    args = parser.parse_args()
    devices = json.loads(run('list', 'devices', '--json'))['devices'].get(RUNTIME, [])
    matches = [d for d in devices if d['name'] == NAME]
    if len(matches) > 1:
        raise SystemExit('Multiple Melee-owned devices found; resolve the duplicate names explicitly.')
    if matches:
        device = matches[0]
        if not device.get('isAvailable') or device.get('deviceTypeIdentifier') != TYPE:
            raise SystemExit('The Melee-owned simulator is unavailable or has an unexpected device type.')
        uuid, state = device['udid'], device['state']
    else:
        if args.shutdown:
            raise SystemExit('No Melee-owned simulator exists; nothing was shut down.')
        uuid, state = run('create', NAME, TYPE, RUNTIME), 'Shutdown'
    if args.boot and state == 'Shutdown':
        run('boot', uuid)
    elif args.shutdown and state == 'Booted':
        run('shutdown', uuid)
    print(uuid)


if __name__ == '__main__':
    main()
