#!/usr/bin/env python3
"""Exercise every retail costume archive through the original native HSD loader."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--build", type=Path, default=Path("native/build"))
    parser.add_argument("--output", type=Path, default=Path("native/build/costume-checks"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    inspect_disc = args.build / "inspect-disc"
    inspect_archive = args.build / "inspect-archive"
    probe = args.build / "aurora-integration/hsd_scene_probe"
    listing = subprocess.check_output([inspect_disc, args.image], text=True)
    # AJ archives contain motions, not costumes. All two-letter color archives
    # are included, including bosses and wireframes present on the retail disc.
    names = re.findall(r"\b(Pl[A-Za-z]{4}\.(?:dat|usd))$", listing, re.MULTILINE)
    names = [name for name in names if name[4:6] != "AJ"]
    if not names:
        raise RuntimeError("No costume archives found")
    results = []
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    for name in names:
        archive = args.output / name
        with archive.open("wb") as file:
            subprocess.run([inspect_disc, args.image, name], stdout=file, check=True)
        symbols = subprocess.check_output([inspect_archive, archive], text=True)
        symbols = [line.split()[1] for line in symbols.splitlines()[1:]]
        joints = [symbol for symbol in symbols if symbol.endswith("_joint")
                  and not symbol.endswith("_matanim_joint")]
        if not joints:
            raise RuntimeError(f"No model root in {name}: {symbols}")
        for joint in joints:
            command = [probe, "--costume", archive, joint]
            material = joint[:-6] + "_matanim_joint"
            if material in symbols:
                command.append(material)
            log = args.output / f"{name}-{joint}.log"
            with log.open("w") as file:
                result = subprocess.run(command, stdout=file, stderr=subprocess.STDOUT, env=env)
            results.append(dict(archive=name, joint=joint, material=material if material in symbols else None,
                                exit_code=result.returncode, log=str(log)))
            print(f"{'PASS' if result.returncode == 0 else 'FAIL'} {name} {joint}", flush=True)
    (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    failed = sum(result["exit_code"] != 0 for result in results)
    print(f"{len(names)} archives, {len(results)} model roots, {failed} failures")
    return int(failed != 0)


if __name__ == "__main__":
    raise SystemExit(main())
