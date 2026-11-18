#!/usr/bin/env python3
"""Bounded Jev controller for the opt-in Melee iOS Simulator agent bridge.

Only structured game observations leave the machine. No screenshots, saves,
assets or credentials are written to the request log. No shell/tool execution
is permitted in model responses. A run records decisions, costs and screenshots;
completion must be verified from game state and screenshots, never model claims.
"""
import argparse
import fcntl
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import time
import urllib.request
from jev_pathfinding import MazeRouter
ROUTERS={}

# buttons, stick x/y, C-stick x/y, number of rendered frames to hold
# Original single-player disables C-stick attacks; use stick flick + A for smashes.
ACTIONS = {
 'wait': (0,0,0,0,0,30), 'confirm': (0x100,0,0,0,0,8),
 'start': (0x1000,0,0,0,0,8), 'back': (0x200,0,0,0,0,8),
 'left': (0,-1,0,0,0,18), 'right': (0,1,0,0,0,18),
 'up': (0,0,1,0,0,8), 'down': (0,0,-1,0,0,8),
 'jump_left': (0x400,-1,0,0,0,18), 'jump_right': (0x400,1,0,0,0,18),
 'jump': (0x400,0,0,0,0,12),
 'attack_left': (0x100,-.6,0,0,0,18), 'attack_right': (0x100,.6,0,0,0,18),
 'smash_left': (0x100,-1,0,0,0,18), 'smash_right': (0x100,1,0,0,0,18),
 'smash_up': (0x100,0,1,0,0,18), 'smash_down': (0x100,0,-1,0,0,18),
 'recover': (0x200,0,1,0,0,24), 'stone': (0x200,0,-1,0,0,24),
 'shield': (0x40,0,0,0,0,18),
}
MACROS={'climb':['jump']*3,'climb_right':['jump_right']*3,'climb_left':['jump_left']*3,
        'combo_left':['smash_left','wait','smash_down','wait','smash_up','wait'],
        'combo_right':['smash_right','wait','smash_down','wait','smash_up','wait']}
CRITERIA = {
 'combo_left':'Perform three strong attacks with time for each animation to finish, starting toward the enemy on the LEFT. Effective against nearby groups while grounded.',
 'combo_right':'Perform three strong attacks with time for each animation to finish, starting toward the enemy on the RIGHT. Effective against nearby groups while grounded.',
 'climb':'Perform three successive vertical jumps to climb toward a platform directly above.',
 'climb_right':'Perform three successive jumps while moving right to climb a high ledge, when the next platform is above the player and jumps remain.',
 'climb_left':'Perform three successive jumps while moving left to climb a high ledge to the left or recover onto the main platform.',
 'left':'Move left toward an enemy or the destination when safely on ground.',
 'right':'Move right toward an enemy or the destination when safely on ground.',
 'jump_left':'Jump and move left to cross a gap, regain the platform, or approach a higher target.',
 'jump_right':'Jump and move right to cross a gap, regain the platform, or approach a higher target.',
 'jump':'Jump vertically to climb; Kirby can jump several times in the air.',
 'attack_left':'Attack an enemy close to the left with a normal attack.',
 'attack_right':'Attack an enemy close to the right with a normal attack.',
 'smash_left':'Use a strong attack to launch a nearby enemy to the left, especially at high damage.',
 'smash_right':'Use a strong attack to launch a nearby enemy to the right, especially at high damage.',
 'smash_up':'Strong upward attack against an enemy immediately above.',
 'smash_down':'Strong sweeping attack against enemies on either side at close range.',
 'recover':'Up special to recover height if falling dangerously low or out of jumps.',
 'stone':'Kirby down special to attack an enemy directly below; avoid this near a pit.',
 'shield':'Briefly block an imminent attack when grounded.',
 'down':'Drop through a passable platform or fall faster to reach a lower objective.',
 'wait':'Let an attack, respawn, cutscene or transition finish.',
}

def state(root):
    p=root/'state.json'
    if time.time()-p.stat().st_mtime>20:
        raise RuntimeError('Simulator observations stopped; inspect the app before sending input.')
    return json.loads(p.read_text())

def act(root, action):
    if action in MACROS:
        initial=state(root)['game']
        for part in MACROS[action]:
            result=act(root,part)
            if action.startswith('combo'):
                game=result['game'];player=next((p for p in game['fighters'] if p['slot']==0),None)
                if (game['mode'],game['scene'])!=(initial['mode'],initial['scene']) or not player or player['airborne'] or player['action']<14:break
        return result
    if action not in ACTIONS: raise ValueError('Unrecognized controller action')
    return act_values(root,ACTIONS[action])

def act_values(root,values):
    before=state(root)
    seq=max(before['sequence'],int(time.time()*1000))+1
    temp=root/'command.tmp'
    temp.write_text(' '.join(map(str,(seq,*values)))+'\n')
    temp.replace(root/'command.txt')
    deadline=time.monotonic()+30
    while time.monotonic()<deadline:
        current=state(root)
        if current['completed']>=seq:return current
        time.sleep(.05)
    raise TimeoutError('Controller action did not complete')

def navigate(root,current):
    g=current['game'];mode=g['mode']
    if mode in (0,24):return act(root,'start')
    if mode in (40,41):return act(root,'confirm')
    if mode==1:
        desired=1 if g['menu']==6 else 0
        return act(root,'confirm' if g['selection']==desired else 'down')
    if mode==4 and g['scene']==112:
        css=g.get('css')
        if not css:raise RuntimeError('Character-select observations unavailable')
        if css['difficulty']!=0:target=css['difficultyDown']
        elif css['stocks']<5:target=css['stocksUp']
        else:target=css['kirby']
        dx=target[0]-css['x'];dy=target[1]-css['y']
        if abs(dx)<1 and abs(dy)<1:
            result=act(root,'confirm')
            if css['stocks']==5 and css['difficulty']==0 and target==css['kirby']:
                time.sleep(.3);return act(root,'start')
            return result
        return act_values(root,(0,max(-1,min(1,dx/5)),max(-1,min(1,dy/5)),0,0,4))
    if mode==4 and not g['fighters']:return act(root,'start')
    raise RuntimeError(f'Unrecognized navigation state: {mode}/{g["scene"]}')

def snapshot(root, device, number):
    subprocess.run(['xcrun','simctl','io',device,'screenshot',str(root/f'frame-{time.time_ns()}-{number:05}.png')],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=15)

def context(current, history):
    game=current['game'];fighters=game['fighters'];player=next((f for f in fighters if f['slot']==0),None)
    enemies=[f for f in fighters if f['slot']!=0 and f['stocks']>0 and f['action']>=14]
    nearest=min(enemies,key=lambda f:abs(f['x']-player['x'])+abs(f['y']-player['y']),default=None) if player else None
    facts={}
    if player and nearest:
        facts=dict(enemy_side='left' if nearest['x']<player['x'] else 'right',
                   enemy_close=abs(nearest['x']-player['x'])<22 and abs(nearest['y']-player['y'])<25,
                   enemy_below=nearest['y']<player['y']-15,
                   enemy_above=nearest['y']>player['y']+15,
                   enemy_high_damage=nearest['damage']>=65)
    if player:facts.update(falling=player['vy']<-.5,low=player['y']<-20,airborne=bool(player['airborne']))
    platforming=game['mode']==4 and game['scene'] in (1,17,27,58,73) and not enemies
    if player and history:
        facts['horizontal_progress_last_action']=history[-1].get('dx',0)
        facts['ascending']=player['vy']>0
        terrain={t['dx']:t['dy'] for t in game.get('terrain',[])}
        facts['gap_right']=terrain.get(20) is None or terrain.get(20,0)<-35
        facts['raised_ground_right']=terrain.get(20) is not None and terrain[20]>8
        landings=[t for t in game.get('terrain',[]) if t['dy'] is not None]
        landing=min(landings,key=lambda t:abs(t['dx'])+abs(t['dy']),default=None)
        facts['nearest_landing_direction']=('left' if landing['dx']<0 else 'right' if landing['dx']>0 else 'below') if landing else 'none_in_range'
        facts['no_floor_below']=terrain.get(0) is None
        facts['high_ledge_right']=terrain.get(20) is not None and terrain[20]>25
        facts['safe_ground_below']=terrain.get(0) is not None and -25<=terrain[0]<=5
        facts['jumps_remaining']=max(0,6-player['jumpsUsed'])
    return dict(goal='Complete Adventure mode on Very Easy using controller input only. Survive and progress.',
                game=game,facts=facts,history=history[-4:],
                strategy=(('Mushroom Kingdom: destination is far RIGHT. Keep moving RIGHT in the air and on ground. Jump RIGHT to clear raised terrain or pits. Do not waste jumps going straight up. If airborne and rising, move RIGHT instead of jumping again. If falling over a gap, jump RIGHT again before getting low; Kirby has six jumps. terrain dx/dy are relative platform sample positions; null means no platform found. Stay above the next platform height. When high_ledge_right is true, choose climb_right to gain height with multiple jumps rather than alternating a single jump and walking into the wall. Defeat Yoshi fighters when they appear.' if game['scene']==1 else 'Platforming section: progress toward the exit, crossing gaps and climbing platforms. Fight enemies blocking progress.') if platforming else 'Combat: defeat all opponents before advancing. If standing on a platform ABOVE an opponent, use down to drop through it and reach them. Move toward the nearest enemy, then use strong attacks when close. If over a gap, jump toward the nearest landing direction immediately. If no landing is in range, return toward the opponents to reach the stage. Kirby has six jumps. Do not walk or drift away from the stage. Terrain and object coordinates are relative to the player.'))

def maze_navigation(root,observation):
    g=observation['game']
    if (g['mode'],g['scene'])!=(4,17):return
    source=root/'maze-markers.json'
    if not source.exists():raise RuntimeError('Maze needs maze-markers.json; extract it from the owned GrNSr.dat with jev_maze_route.py.')
    markers=json.loads(source.read_text());progress=root/'maze-progress.json'
    data=json.loads(progress.read_text()) if progress.exists() else {'visited':[],'target':None}
    player=next((p for p in g['fighters'] if p['slot']==0),None)
    if not player:return
    if player['stocks']>data.get('lastStocks',player['stocks']):data={'visited':[],'target':None}
    data['lastStocks']=player['stocks']
    enemies=[p for p in g['fighters'] if p['slot']!=0 and p['stocks']>0 and p['action']>=14]
    remaining=[m for m in markers if m['id'] not in data['visited']]
    if not remaining:raise RuntimeError('All maze markers visited without finding the exit; inspect navigation evidence.')
    target=next((m for m in remaining if m['id']==data['target']),None)
    if not target:
        target=min(remaining,key=lambda m:abs(m['x']-player['x'])+abs(m['y']-player['y']));data['target']=target['id']
    dx=target['x']-player['x'];dy=target['y']-player['y']
    if abs(dx)<12 and abs(dy)<18 and not player['airborne']:
        data['visited'].append(target['id']);data['target']=None
    progress.write_text(json.dumps(data))
    if not enemies:
        geometry=root/'maze-geometry.json'
        if geometry.exists():
            if str(geometry) not in ROUTERS:ROUTERS[str(geometry)]=MazeRouter(geometry)
            wx,wy=ROUTERS[str(geometry)].waypoint((player['x'],player['y']),(target['x'],target['y']))
            dx=wx-player['x'];dy=wy-player['y']
        observation['navigation']=dict(target=target,dx=round(dx,1),dy=round(dy,1),visited=data['visited'])
        observation['strategy']='Underground Maze: follow the intermediate corridor waypoint given by navigation dx/dy to reach the marker on the ground. The waypoint routes around solid floors and walls; it can temporarily point away from the final marker. Some markers trigger Link fights, one is the exit. Use the navigation dx/dy: negative dx means LEFT, positive dx RIGHT, positive dy UP, negative dy DOWN. Descend to a lower marker rather than repeatedly jumping higher. Fight any spawned Link before continuing.'

def ascent_navigation(observation):
    g=observation['game']
    if (g['mode'],g['scene'])==(4,58):
        observation['strategy']='F-Zero course: the exit is far RIGHT. Race cars periodically sweep the road and cause severe knockback. Favor jumping RIGHT above the road and moving RIGHT through the air. Climb RIGHT when low to avoid traffic, then keep moving right without wasting further jumps while rising. Land on elevated platforms to replenish Kirby jumps. Do not stop on the low road or attack empty space.'
    if g['mode']!=4 or g.get('stage') not in (33,22):return # Brinstar escape / Icicle Mountain
    if any(p['slot']!=0 and p['stocks']>0 for p in g['fighters']):return
    platforms=[t for t in g.get('terrain',[]) if t['dy'] is not None and t['dy']>12]
    if platforms:
        target=min(platforms,key=lambda t:t['dy']+abs(t['dx'])*.5)
        observation['ascent']=target
    elif g['stage']==22:
        observation['ascent']=dict(dx=0,dy=0)
    observation['strategy']='Climb UP to escape or reach the mountain summit. Jump toward the next higher platform. Land to replenish Kirby jumps. Avoid wasting all jumps before reaching a platform.'

def available_actions(observation):
    """Expose context-appropriate controls; Jev still chooses every gameplay action."""
    g=observation['game'];f=observation['facts']
    player=next((p for p in g['fighters'] if p['slot']==0),None)
    if not player:return {'wait':CRITERIA['wait']}
    enemies=[p for p in g['fighters'] if p['slot']!=0 and p['stocks']>0 and p['action']>=14]
    nearest=min(enemies,key=lambda p:abs(p['x']-player['x'])+abs(p['y']-player['y']),default=None)
    remaining=max(0,6-player['jumpsUsed'])
    if g['scene']==58 and not enemies:
        # Spend the useful arc of each jump moving forward before jumping again.
        # Repeated climb macros exhaust Kirby's jumps above the starting edge.
        names=['right']
        if player['action']<14:names=['wait']
        elif not player['airborne'] or player['vy']<-.5:
            if remaining:names+=['jump_right']
            elif f.get('no_floor_below'):names+=['recover']
    elif (f.get('no_floor_below') or f.get('low')) and player['airborne'] and f.get('falling'):
        direction=f.get('nearest_landing_direction')
        vertical=direction=='below'
        if direction not in ('left','right'):
            direction=('left' if nearest['x']<player['x'] else 'right') if nearest else 'right'
        names=[direction,'recover']
        if remaining:names+=['jump' if vertical else 'jump_'+direction,'climb' if vertical else 'climb_'+direction]
    elif nearest:
        dx=nearest['x']-player['x'];dy=nearest['y']-player['y'];direction='left' if dx<0 else 'right'
        if dy>25:
            names=[direction,'smash_up','recover']
            if remaining:names+=['jump_'+direction]
            if dy>45 and remaining>1:names+=['climb_'+direction]
        elif dy<-25:
            names=([direction,'down','wait'] if not player['airborne'] else [direction,'attack_'+direction,'smash_down','stone','wait'])
        elif abs(dx)>25:
            names=[direction,'attack_'+direction]
            if remaining:names+=['jump_'+direction]
        else:
            names=['smash_'+direction,'attack_'+direction,'smash_up','smash_down','shield','stone']
            if not player['airborne']:names+=['combo_'+direction]
    elif 'ascent' in observation:
        target=observation['ascent'];direction='left' if target['dx']<0 else 'right'
        names=['wait'] if target['dy']<=12 else [direction,'wait','recover']
        if remaining and target['dy']>12:
            names+=['jump' if abs(target['dx'])<15 else 'jump_'+direction]
            if remaining>1:names+=['climb' if abs(target['dx'])<15 else 'climb_'+direction]
    elif 'navigation' in observation:
        nav=observation['navigation'];direction='left' if nav['dx']<0 else 'right'
        names=[direction,'attack_'+direction,'wait']
        if nav['dy']<-20:names+=['down']
        elif remaining:names+=['jump_'+direction]
        if nav['dy']>35 and remaining>1:names+=['climb_'+direction]
    else:
        names=['right','attack_right','wait']
        if remaining:names+=['jump_right']
        if remaining>1 and (f.get('high_ledge_right') or g['scene']==58):names+=['climb_right']
    return {name:CRITERIA[name] for name in names}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('directory',type=Path)
    p.add_argument('--device',required=True)
    p.add_argument('--action',choices=list(ACTIONS)+list(MACROS))
    p.add_argument('--model',default='typesafe/jev-1.13')
    p.add_argument('--max-cost',type=float,default=2)
    p.add_argument('--max-calls',type=int,default=3000)
    p.add_argument('--max-seconds',type=float,default=3600)
    a=p.parse_args();root=a.directory.resolve()
    lock=(root/'controller.lock').open('w')
    try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    except BlockingIOError:
        lock.close();raise SystemExit('Another controller owns this run directory.')
    try:
        if not math.isfinite(a.max_cost) or a.max_cost<=0:p.error('Cost limit must be positive and finite')
        if a.action:
            print(json.dumps(act(root,a.action)));snapshot(root,a.device,0);return
        key=os.environ.get('OPEN_ROUTER_API_KEY') or os.environ.get('OPENROUTER_API_KEY')
        if not key:raise SystemExit('Set OPEN_ROUTER_API_KEY or OPENROUTER_API_KEY in the environment.')
        cost=sum(json.loads(line).get('charge',json.loads(line).get('reservedCost',0))
                 for path in root.glob('decisions-*.jsonl') for line in path.read_text().splitlines())
        calls=0;history=[];started=time.monotonic();last_scene=None;last_snapshot=-1;last_fighters=None;frozen=0;cleared=False
        run_id=time.time_ns()
        (root/f'run-{run_id}.json').write_text(json.dumps(dict(model=a.model,maxCost=a.max_cost,maxCalls=a.max_calls,maxSeconds=a.max_seconds,harnessSHA256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2))
        with (root/f'decisions-{run_id}.jsonl').open('w') as log:
            while calls<a.max_calls and time.monotonic()-started<a.max_seconds and cost+.01<=a.max_cost:
                current=state(root);g=current['game'];scene=(g['mode'],g['scene'])
                signature=(scene,json.dumps(g['fighters'],sort_keys=True))
                frozen=frozen+1 if signature==last_fighters else 0
                last_fighters=signature
                if frozen>=8 and g['fighters']:
                    snapshot(root,a.device,calls);act(root,'start');frozen=0;continue
                if scene!=last_scene or (calls%50==0 and calls!=last_snapshot):
                    snapshot(root,a.device,calls);last_scene=scene;last_snapshot=calls
                if g['mode']==22:
                    # Let the clear transition render into staff credits, both
                    # for visible evidence and ending-specific shader coverage.
                    for _ in range(4):ending=act_values(root,(0,0,0,0,0,180))
                    snapshot(root,a.device,calls)
                    cleared=True
                    (root/'completion.json').write_text(json.dumps(dict(status='adventure_clear_ending',state=current,endingState=ending,calls=calls,cost=cost,runId=run_id),indent=2))
                    print('Adventure clear ending reached.',flush=True);break
                if g['mode']!=4 or not g['fighters']:
                    navigate(root,current);time.sleep(.15);continue
                observation=context(current,history)
                maze_navigation(root,observation)
                ascent_navigation(observation)
                criteria=available_actions(observation)
                body=json.dumps(dict(model=a.model,state=observation,questions={'action':dict(type='choice',instructions='Choose the safest useful next controller action to progress toward the goal, considering game state and recent history. Avoid repeatedly using an action that failed to move the player.',criteria=criteria)})).encode()
                if len(body)>32000:raise RuntimeError('Decision context exceeds the bounded request size')
                request=urllib.request.Request('https://openrouter.ai/api/alpha/decisions',body,{'Authorization':'Bearer '+key,'Content-Type':'application/json'},method='POST')
                for attempt in range(3):
                    try:
                        with urllib.request.urlopen(request,timeout=30) as response:result=json.load(response)
                        charge=float(result['usage']['cost']);choice=result['answers']['action']
                        if not math.isfinite(charge) or charge<0 or choice['type']!='choice' or choice['choice'] not in criteria:raise ValueError('Invalid decision or usage response')
                        break
                    except BaseException as error:
                        # Reserve a possibly billed request before any bounded retry.
                        code=getattr(error,'code',None)
                        failure=dict(error=type(error).__name__,httpStatus=code,reservedCost=.01,attempt=attempt+1)
                        cost+=.01;log.write(json.dumps(failure)+'\n');log.flush()
                        print(json.dumps(failure),flush=True)
                        retryable=isinstance(error,TimeoutError) or code in (429,500,502,503,504)
                        if not retryable or attempt==2 or cost+.01>a.max_cost:
                            raise RuntimeError(f'Decision request failed ({type(error).__name__}, status {code}); stopped without exposing credentials.') from None
                        time.sleep(2*(attempt+1))
                cost+=charge;calls+=1;action=choice['choice']
                item=dict(call=calls,cost=cost,charge=charge,action=action,confidence=choice.get('confidence'),before=current)
                try:after=act(root,action)
                except BaseException as error:
                    item['error']=type(error).__name__;log.write(json.dumps(item)+'\n');log.flush();raise
                item['after']=after
                log.write(json.dumps(item)+'\n');log.flush()
                player=next((f for f in after['game']['fighters'] if f['slot']==0),{})
                previous=next((f for f in current['game']['fighters'] if f['slot']==0),{})
                history.append(dict(action=action,x=player.get('x'),y=player.get('y'),damage=player.get('damage'),dx=round(player.get('x',0)-previous.get('x',0),2)))
                if calls%20==0:print(json.dumps(dict(calls=calls,cost=cost,scene=scene,player=player)),flush=True)
        print(json.dumps(dict(status='adventure_clear_ending' if cleared else 'stopped',calls=calls,cost=cost,seconds=time.monotonic()-started)),flush=True)
    finally:lock.close()
if __name__=='__main__':main()
