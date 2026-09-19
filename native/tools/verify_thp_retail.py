#!/usr/bin/env python3
"""Check retail THP pixels against libjpeg using the separately built verifier."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    args = parser.parse_args()
    inspect = ROOT / 'native/build/inspect-disc'
    verify = ROOT / 'native/build/verify-thp-pixels'
    target = ROOT / 'native/build/movie-validation.thp'
    listing = subprocess.check_output([inspect, args.image], text=True)
    names = [line.split()[-1] for line in listing.splitlines() if line.endswith('.thp')]

    def check(name, data):
        target.write_bytes(data)
        result = subprocess.run([verify, target], capture_output=True, text=True)
        print(name, result.stdout.strip(), result.stderr.strip(), flush=True)
        result.check_returncode()

    for name in names:
        check(name, subprocess.check_output([inspect, args.image, name]))
    movie = subprocess.check_output([inspect, args.image, 'MvOpen.mth'])
    def word(offset):
        return int.from_bytes(movie[offset:offset + 4], 'big')
    assert movie[:4] == b'MTHP' and word(8) == 2
    count, offset, size = word(28), word(32), word(40)
    assert count > 0
    selected = {i * (count - 1) // 19 for i in range(20)}
    for index in range(count):
        assert size >= 4 and offset <= len(movie) - size
        if index in selected:
            frame = movie[offset + 4:offset + size]
            # MTH frames include alignment padding after the JPEG EOI.
            end = frame.rfind(b'\xff\xd9')
            assert end >= 0
            check(f'MvOpen frame {index}/{count}', frame[:end + 2])
        next_size = word(offset)
        offset, size = offset + size, next_size
    print(f'PASS {len(names)} stills and {len(selected)} opening frames; '
          f'validated all {count} packed frame extents', flush=True)


if __name__ == '__main__':
    main()
