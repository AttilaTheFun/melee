#!/usr/bin/env python3
"""Create a SMALL NONPLAYABLE ISO test fixture from a user's disc. Never distribute it."""
from pathlib import Path
import struct
import subprocess
import sys
native=Path(__file__).resolve().parents[1]
disc=Path(sys.argv[1]).resolve()
output=Path(sys.argv[2]).resolve()
files={}
for code in ['Mr','Fx','Pe']:
    for suffix in ['Nr','AJ','']:
        name=f'Pl{code}{suffix}.dat'
        files[name]=subprocess.check_output([str(native/'build/inspect-disc'),str(disc),'/'+name])
count=len(files)+1
strings=b''
records=[]
offset=0x1000
for name,data in files.items():
    records.append(struct.pack('>III',len(strings),offset,len(data)))
    strings+=name.encode()+b'\0'
    offset=(offset+len(data)+31)&~31
fst=struct.pack('>III',0x1000000,0,count)+b''.join(records)+strings
assert len(fst)<0x800
header=bytearray(0x1000);header[:6]=b'GALE01'
struct.pack_into('>I',header,0x1c,0xc2339f3d)
struct.pack_into('>II',header,0x424,0x800,len(fst))
header[0x800:0x800+len(fst)]=fst
with output.open('wb') as file:
    file.write(header)
    for record,data in zip(records,files.values()):
        file.seek(struct.unpack('>III',record)[1]);file.write(data)
print(output)
