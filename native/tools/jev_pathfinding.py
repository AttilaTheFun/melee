"""Read-only coarse corridor routing; Jev chooses the controller actions.

The occupancy image is geometric working data, not a game screenshot. It avoids
routing straight through solid maze floors. Moving platforms and combat remain
handled by live observations and the policy. Pillow is used only locally.
"""
import heapq
import json
import math
from functools import lru_cache
from PIL import Image, ImageDraw

class MazeRouter:
    def __init__(self,path):
        data=json.loads(path.read_text());segments=data['segments']
        values=[p for line in segments for p in line]
        self.minx=math.floor(min(p[0] for p in values))-30
        self.miny=math.floor(min(p[1] for p in values))-30
        width=math.ceil((max(p[0] for p in values)-self.minx+30)/2)
        height=math.ceil((max(p[1] for p in values)-self.miny+30)/2)
        if width>2000 or height>2000:raise ValueError('Collision map exceeds bounds')
        self.image=Image.new('L',(width,height),255);draw=ImageDraw.Draw(self.image)
        for a,b in segments:draw.line([self.pixel(a),self.pixel(b)],fill=0,width=4)
        self.pixels=self.image.load();self.width=width;self.height=height

    def pixel(self,p):return ((p[0]-self.minx)/2,(p[1]-self.miny)/2)
    def world(self,p):return (p[0]*2+self.minx,p[1]*2+self.miny-10)
    def free(self,p):
        x,y=map(round,p)
        return 0<x<self.width-1 and 0<y<self.height-1 and self.pixels[x,y]>0
    def clear(self,a,b):
        n=max(1,math.ceil(max(abs(a[0]-b[0]),abs(a[1]-b[1]))))
        return all(self.free((a[0]+(b[0]-a[0])*i/n,a[1]+(b[1]-a[1])*i/n)) for i in range(n+1))
    def nearest(self,p):
        x,y=self.pixel((p[0],p[1]+10));base=(round(x/5)*5,round(y/5)*5)
        candidates=[(base[0]+dx*5,base[1]+dy*5) for dx in range(-3,4) for dy in range(-3,4)]
        return min((q for q in candidates if self.free(q)),key=lambda q:(q[0]-x)**2+(q[1]-y)**2,default=None)
    @lru_cache(maxsize=30000)
    def neighbors(self,p):
        return tuple((p[0]+dx,p[1]+dy) for dx,dy in ((5,0),(-5,0),(0,5),(0,-5),(5,5),(5,-5),(-5,5),(-5,-5)) if self.clear(p,(p[0]+dx,p[1]+dy)))
    def waypoint(self,position,target):
        start=self.nearest(position);end=self.nearest(target)
        if start is None or end is None:return target
        queue=[(0,start)];cost={start:0};parent={};found=False
        while queue and len(cost)<30000:
            _,current=heapq.heappop(queue)
            if current==end:found=True;break
            for neighbor in self.neighbors(current):
                value=cost[current]+math.dist(current,neighbor)
                if value<cost.get(neighbor,float('inf')):
                    cost[neighbor]=value;parent[neighbor]=current
                    heapq.heappush(queue,(value+math.dist(neighbor,end),neighbor))
        if not found:return target
        route=[end]
        while route[-1]!=start:route.append(parent[route[-1]])
        route.reverse();next_point=start
        for point in route[1:]:
            if math.dist(start,point)>32 or not self.clear(start,point):break
            next_point=point
        return self.world(next_point)
