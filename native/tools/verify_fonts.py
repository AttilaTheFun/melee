#!/usr/bin/env python3
"""Verify original native HSD font objects against bytes in the user's DOL."""
import subprocess
from prepare_fonts import ROOT, DEFAULT_DOL, FONTS, prepare, extract


def main():
    generated = prepare()
    executable = ROOT / 'native/build/test-font-bytes'
    command = ['xcrun', 'clang', '-DMELEE_NATIVE', '-std=gnu11', '-O2',
               '-Isrc', '-Iextern/dolphin/include', '-I' + str(generated),
               '-Wl,-dead_strip', 'native/tests/font_bytes.c',
               'src/sysdolphin/baselib/sislib_font.c', 'src/sysdolphin/baselib/hsd_3915.c',
               '-o', str(executable)]
    subprocess.run(command, cwd=ROOT, check=True)
    actual = subprocess.check_output([str(executable)])
    dol = DEFAULT_DOL.read_bytes()
    expected = b''.join(extract(dol, address, size) for _, address, size, _ in FONTS)
    if actual != expected:
        raise AssertionError('Compiled font object bytes differ from the original DOL')
    print(f'Original HSD font objects preserve all {len(actual):,} bytes (287 text and 128 debug glyphs).')


if __name__ == '__main__':
    main()
