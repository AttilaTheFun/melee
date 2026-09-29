import assert from 'node:assert/strict';
import {GameSession,fingerprintFile} from './netplay/session.mjs';
import {NEUTRAL} from './netplay/inputs.mjs';
function pair(overrides={}){
 const transports=[0,1].map(()=>({ready:Promise.resolve(),closed:false,onClose:()=>{},
   close(reason){if(this.closed)return;this.closed=true;this.onClose(reason);},
 }));
 transports.forEach((t,i)=>{t.send=packet=>{if(t.closed)throw new Error('closed');const copy=packet.slice();queueMicrotask(()=>transports[1-i].onPacket(copy));};});
 const sessions=transports.map((transport,localSlot)=>new GameSession({transport,localSlot,seed:123,delay:2,build:'a'.repeat(64),disc:'b'.repeat(64),...(localSlot?overrides:{})}));
 return sessions;
}
const good=pair();await Promise.all(good.map(peer=>peer.start()));
for(let frame=0;frame<1000;frame++){
 const result=await Promise.all(good.map((peer,slot)=>peer.nextInput([slot?512:256,0,0,0,0,0,0,1])));
 assert.deepEqual(result[0],result[1]);
 const state={netTick:frame+1,mode:2,scene:2,rng:123,stageGuidance:0,cursorTargets:[[0,0],[0,0]],fighters:[[1,0,frame,0,14,0,0],[1,1,0,0,14,0,1]]};
 good.forEach(peer=>peer.observeState(state));
 await Promise.resolve();
}
assert.equal(good[0].lastVerified,960);
const mismatchState={netTick:1020,mode:2,scene:2,rng:123,stageGuidance:0,cursorTargets:[[0,0],[0,0]],fighters:[[1,0,1,0,14,0,0],[1,1,0,0,14,0,1]]};
good[0].observeState(mismatchState);good[1].observeState({...mismatchState,rng:124});await Promise.resolve();
assert.ok(good.every(peer=>peer.closed));
assert.match(good[0].reason,/desynchronized/);
good.forEach(peer=>peer.close());await assert.rejects(good[0].nextInput(NEUTRAL));
for(const mismatch of [{seed:124},{delay:3},{build:'c'.repeat(64)},{disc:'d'.repeat(64)},{localSlot:0}]){
 const peers=pair(mismatch);
 const outcomes=await Promise.allSettled(peers.map(peer=>peer.start()));
 assert.ok(outcomes.every(result=>result.status==='rejected'));assert.ok(peers.every(peer=>peer.closed));
}
const empty=await fingerprintFile(new Blob([]));
const small=await fingerprintFile(new Blob(['melee-test']));
assert.notEqual(empty,small);assert.equal(small,await fingerprintFile(new Blob(['melee-','test'])));
console.log('PASS session handshake mismatches, player-slot validation, 1000 paired ticks, cleanup and file fingerprints');
