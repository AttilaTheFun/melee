#!/usr/bin/env python3
"""Merge captured Aurora configuration catalogs into a portable app seed.

Only configuration rows are copied: no Dawn binaries, game data, paths, or logs.
Use SQLite backups of live catalogs (or captured DB plus WAL), never omit WAL.
"""
import argparse
from pathlib import Path
import sqlite3

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
p.add_argument('catalogs', type=Path, nargs='+')
a = p.parse_args()
if a.output.exists():
    p.error('Choose a new output path; existing catalogs are not overwritten.')
rows = {}
for path in a.catalogs:
    with sqlite3.connect(path.resolve().as_uri()+'?mode=ro', uri=True) as db:
        if db.execute('PRAGMA integrity_check').fetchone() != ('ok',):
            raise SystemExit(f'Invalid SQLite catalog: {path}')
        if db.execute('SELECT value FROM aurora_schema').fetchall() != [(1,)]:
            raise SystemExit(f'Unsupported catalog schema: {path}')
        for row in db.execute('SELECT type,hash,config_version,config_size,config,first_frame_used FROM pipeline_cache'):
            kind, hash_, version, size, config, first = row
            if kind not in (0,1) or not 0 <= first <= 0xffffffff or size != len(config) or size <= 0:
                raise SystemExit(f'Invalid configuration in {path}')
            key = (kind, hash_)
            if key in rows:
                previous = rows[key]
                if previous[2:5] != row[2:5]:
                    raise SystemExit(f'Conflicting shader configuration: {key}')
                first = min(first, previous[5])
            rows[key] = (*row[:5], first)
a.output.parent.mkdir(parents=True, exist_ok=True)
with sqlite3.connect(a.output) as out:
    out.executescript('''CREATE TABLE aurora_schema(value INTEGER);
INSERT INTO aurora_schema VALUES(1);
CREATE TABLE pipeline_cache(type INTEGER NOT NULL, hash INTEGER NOT NULL,
config_version INTEGER NOT NULL, config_size INTEGER NOT NULL, config BLOB NOT NULL,
first_frame_used INTEGER NOT NULL, PRIMARY KEY(type,hash));''')
    out.executemany('INSERT INTO pipeline_cache VALUES(?,?,?,?,?,?)', [rows[k] for k in sorted(rows)])
print(f'{len(rows)} unique shader configurations → {a.output}')
