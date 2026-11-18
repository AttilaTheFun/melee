#!/usr/bin/env python3
"""Extract only six navigation marker coordinates from the owner's GrNSr.dat.

This does not reveal which randomized marker is the exit, change game state,
or copy models/textures. Refuse unsupported non-identity ancestor transforms.
"""
import argparse
import json
from pathlib import Path
import struct


def extract(path, geometry=False):
    b=path.read_bytes()
    def u(at):return struct.unpack_from('>I',b,at)[0]
    def d(at):
        if not 0<=at<=data_size-4:raise ValueError('Archive offset out of bounds')
        return u(at+32)
    def vec(at):return struct.unpack_from('>3f',b,at+32)
    if len(b)<32 or u(0)!=len(b):raise ValueError('Invalid archive length')
    data_size,nr,np,ne=[u(x) for x in (4,8,12,16)]
    public=32+data_size+4*nr;strings=public+8*(np+ne)
    if strings>len(b) or np>4096:raise ValueError('Invalid archive tables')
    symbols={b[strings+u(public+8*i+4):].split(b'\0')[0]:u(public+8*i) for i in range(np)}
    root=symbols[b'map_head'];table=d(root);count=d(root+4);markers=[]
    if count>256:raise ValueError('Too many point maps')
    for i in range(count):
        joint,pairs,num=[d(table+12*i+k) for k in (0,4,8)];joints=[];seen=set()
        def walk(at,parents):
            while at:
                if at in seen or len(seen)>4096 or len(parents)>100:raise ValueError('Invalid joint hierarchy')
                seen.add(at);joints.append((at,parents));walk(d(at+8),parents+[at]);at=d(at+12)
        walk(joint,[])
        if num>4096:raise ValueError('Too many markers')
        for k in range(num):
            index,kind=struct.unpack_from('>hh',b,32+pairs+4*k)
            if 189<=kind<=194:
                at,parents=joints[index];position=[0.,0.,0.]
                for p in parents+[at]:
                    if vec(p+20)!=(0.,0.,0.) or vec(p+32)!=(1.,1.,1.):raise ValueError('Unsupported marker transform')
                    position=[a+c for a,c in zip(position,vec(p+44))]
                markers.append(dict(id=kind,x=round(position[0],2),y=round(position[1],2)))
    if {m['id'] for m in markers}!=set(range(189,195)) or len(markers)!=6:raise ValueError('Missing/duplicate maze markers')
    if geometry:
        root=symbols[b'coll_data'];v,nv,l,nl=[d(root+k) for k in (0,4,8,12)]
        if nv>2048 or nl>1536:raise ValueError('Invalid collision counts')
        vertices=[struct.unpack_from('>2f',b,32+v+8*i) for i in range(nv)]
        segments=[]
        for i in range(nl):
            a,c=struct.unpack_from('>2H',b,32+l+16*i)
            if a>=nv or c>=nv:raise ValueError('Invalid collision index')
            segments.append([list(vertices[a]),list(vertices[c])])
        return dict(markers=markers,segments=segments)
    return markers

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    a.output.write_text(json.dumps(extract(a.archive),indent=2)+'\n')
