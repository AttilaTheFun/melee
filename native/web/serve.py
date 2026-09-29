#!/usr/bin/env python3
"""Serve only the asset-free build files on loopback. Ctrl-C stops the server."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[1] / 'build/web'
FILES = {'index.html','app.js','renderer.js','worker.js','scene-data.js','melee.mjs','melee.wasm'}
class Handler(SimpleHTTPRequestHandler):
    def do_GET(self):
        name = urlsplit(self.path).path
        if name == '/':
            self.path = '/index.html'
        elif name.lstrip('/') not in FILES or name.count('/') != 1:
            self.send_error(404)
            return
        super().do_GET()
    def do_HEAD(self):
        if urlsplit(self.path).path not in {'/'} | {'/'+f for f in FILES}:
            self.send_error(404)
            return
        super().do_HEAD()
    def end_headers(self):
        self.send_header('Cache-Control','no-store')
        # Single worker, no SAB/pthreads: COOP/COEP are not needed for this slice.
        super().end_headers()

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port',type=int,default=8080)
args=parser.parse_args()
if any(not (ROOT / f).is_file() for f in FILES):
    raise SystemExit('Build first: python3 native/web/build.py')
server=ThreadingHTTPServer(('127.0.0.1',args.port),partial(Handler,directory=str(ROOT)))
print(f'Open http://localhost:{args.port} in desktop Safari. Ctrl-C stops the server.',flush=True)
try:
    server.serve_forever()
except KeyboardInterrupt:
    pass
finally:
    server.server_close()
