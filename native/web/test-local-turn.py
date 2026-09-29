#!/usr/bin/env python3
"""Exercise real loopback TURN, close all listeners and remove credentials on exit."""
import argparse
from contextlib import ExitStack
import json
import os
from pathlib import Path
import secrets
import signal
import shutil
import socket
import struct
import subprocess
import tempfile
import time

web = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--game', action='store_true')
parser.add_argument('--ticks', type=int, default=2200)
parser.add_argument('--loss-percent', type=float, default=0,
                    help='Drop this percentage of actual TURN UDP datagrams in both directions')
args = parser.parse_args()
if not 0 <= args.loss_percent <= 30:
    parser.error('--loss-percent must be between 0 and 30')
def terminate(signum, frame):
    raise SystemExit(128 + signum)
signal.signal(signal.SIGTERM, terminate)
server = shutil.which('turnserver')
if not server:
    raise SystemExit('Install coturn to run this test (macOS: brew install coturn).')
with tempfile.TemporaryDirectory(prefix='melee-turn-') as directory, ExitStack() as resources:
    root = Path(directory)
    password = secrets.token_hex(32)
    # Ask the OS for an available UDP port. Coturn startup is checked below.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
        probe.bind(('127.0.0.1', 0))
        port = probe.getsockname()[1]
    config = root / 'turn.conf'
    config.write_text('\n'.join([
        'listening-ip=127.0.0.1', 'relay-ip=127.0.0.1', f'listening-port={port}',
        'min-port=49400', 'max-port=49430', 'allow-loopback-peers',
        'no-multicast-peers', 'no-cli', 'no-tls', 'no-dtls', 'no-tcp',
        'lt-cred-mech', 'realm=melee-local-test', f'user=melee-test:{password}',
        'relay-threads=1', 'total-quota=8', 'user-quota=8',
        f'pidfile={root / "turn.pid"}', f'userdb={root / "turn.sqlite"}',
        'log-file=stdout', 'simple-log',
    ]) + '\n')
    config.chmod(0o600)
    ice = root / 'ice.json'
    ice.write_text(json.dumps([{'urls': [f'turn:127.0.0.1:{port}?transport=udp'],
                                'username': 'melee-test', 'credential': password}]))
    ice.chmod(0o600)
    with (root / 'turn.log').open('w+') as log:
        process = subprocess.Popen([server, '-c', str(config)], stdout=log, stderr=subprocess.STDOUT)
        try:
            ready = False
            for _ in range(50):
                if process.poll() is not None:
                    raise RuntimeError('Loopback TURN exited during startup')
                transaction = secrets.token_bytes(12)
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
                    probe.settimeout(0.1)
                    probe.sendto(struct.pack('!HHI', 1, 0, 0x2112a442) + transaction, ('127.0.0.1', port))
                    try:
                        response, _ = probe.recvfrom(2048)
                        ready = response[:2] == b'\x01\x01' and response[8:20] == transaction
                    except TimeoutError:
                        pass
                if ready:
                    break
                time.sleep(0.1)
            if not ready:
                raise RuntimeError('Loopback TURN did not answer STUN readiness check')
            proxy = None
            if args.loss_percent:
                from udp_impairment import LossyTurnProxy
                proxy = LossyTurnProxy(port, args.loss_percent)
                resources.callback(proxy.close)
                settings = json.loads(ice.read_text())
                settings[0]['urls'] = [f'turn:127.0.0.1:{proxy.port}?transport=udp']
                ice.write_text(json.dumps(settings))
            env = dict(os.environ, MELEE_ICE_CONFIG=str(ice), MELEE_RELAY_ONLY='1',
                       MELEE_NET_TICKS=str(args.ticks))
            test = 'test-net-game.mjs' if args.game else 'test-net-transport.mjs'
            subprocess.run(['node', str(web / test)], env=env, check=True)
            if proxy and not proxy.dropped:
                raise RuntimeError('Loss test did not actually drop any UDP datagrams')
        finally:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            print('Loopback TURN stopped; temporary credentials removed on exit.')
