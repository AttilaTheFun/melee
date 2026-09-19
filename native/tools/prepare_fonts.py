#!/usr/bin/env python3
"""Extract HSD font bytes from the user's verified GALE01 revision-2 DOL.

Generated files stay under ignored native/build; no game data is downloaded.
The ranges and glyph sizes match config/GALE01/symbols.txt and HSD headers.
"""
from pathlib import Path
import hashlib
import json
import struct
import argparse
import tempfile

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DOL = ROOT / 'orig/GALE01/sys/main.dol'
DOL_SHA1 = '08e0bf20134dfcb260699671004527b2d6bb1a45'
FONTS = [('sislib_font', 0x8040CD40, 0x23E00, 512),
         ('debug_font', 0x804088B8, 0x1C00, 56)]


def extract(data, address, length):
    if len(data) < 0x100:
        raise ValueError('Truncated DOL header')
    matches = []
    for i in range(18):
        offset = struct.unpack_from('>I', data, i * 4)[0]
        base = struct.unpack_from('>I', data, 0x48 + i * 4)[0]
        size = struct.unpack_from('>I', data, 0x90 + i * 4)[0]
        if size and base <= address and address + length <= base + size:
            start = offset + address - base
            if offset < 0x100 or offset + size > len(data):
                raise ValueError('Invalid DOL section extent')
            matches.append(data[start:start + length])
    if len(matches) != 1:
        raise ValueError('Font range must belong to exactly one DOL section')
    return matches[0]


def write_generated(path, data):
    if path.is_file() and path.read_bytes() == data:
        return
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as temporary:
        name = Path(temporary.name)
        try:
            temporary.write(data)
            temporary.flush()
        except BaseException:
            name.unlink(missing_ok=True)
            raise
    try:
        name.replace(path)
    finally:
        name.unlink(missing_ok=True)


def prepare(dol=DEFAULT_DOL):
    data = Path(dol).read_bytes()
    if hashlib.sha1(data).hexdigest() != DOL_SHA1:
        raise ValueError('Expected the verified GALE01 revision-2 executable')
    output = ROOT / 'native/build/generated'
    folder = output / 'sysdolphin/baselib'
    folder.mkdir(parents=True, exist_ok=True)
    manifest = {'dol_sha1': DOL_SHA1, 'fonts': []}
    for name, address, length, glyph_size in FONTS:
        raw = extract(data, address, length)
        assert length % glyph_size == 0
        lines = ['/* Generated from the user-provided game; do not commit. */']
        for start in range(0, length, glyph_size):
            lines.append('{{')
            for row in range(start, start + glyph_size, 16):
                lines.append('    ' + ', '.join(f'0x{x:02x}' for x in raw[row:min(row+16,start+glyph_size)]) + ',')
            lines.append('}},')
        write_generated(folder / (name + '.inc'), ('\n'.join(lines) + '\n').encode())
        write_generated(folder / (name + '.bin'), raw)
        manifest['fonts'].append({'name': name, 'address': address, 'bytes': length,
                                  'glyphs': length // glyph_size, 'sha256': hashlib.sha256(raw).hexdigest()})
    write_generated(output / 'fonts.json', (json.dumps(manifest, indent=2) + '\n').encode())
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dol', type=Path, default=DEFAULT_DOL)
    print(prepare(parser.parse_args().dol))
