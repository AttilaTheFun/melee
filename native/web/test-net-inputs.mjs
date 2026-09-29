import assert from 'node:assert/strict';
import {InputTimeline,encodeInput,decodeInput,NEUTRAL} from './netplay/inputs.mjs';
const input=[256,-1,0.25,0.1,-0.8,0.7,1,1];
assert.deepEqual(decodeInput(encodeInput(0xffffffff,input)),{frame:0xffffffff,pad:input.map(Math.fround)});
for(const value of [NaN,Infinity,-Infinity])assert.throws(()=>encodeInput(1,[0,value,0,0,0,0,0,1]));
assert.throws(()=>encodeInput(-1,NEUTRAL));assert.throws(()=>decodeInput(new Uint8Array(39)));
const bad=encodeInput(1,NEUTRAL);bad[0]^=1;assert.throws(()=>decodeInput(bad));
const peers=[0,1].map(localSlot=>new InputTimeline({localSlot,delay:2}));
for(let frame=0;frame<10000;frame++){
  const pads=[[frame%2?256:0,0.5,0,0,0,0,0,1],[512,-0.25,0,0,0,0.9,0,1]];
  const packets=peers.map((peer,i)=>peer.sample(pads[i]));
  assert.throws(()=>peers[0].sample(NEUTRAL));
  peers[1].receive(packets[0]);peers[0].receive(packets[1]);peers[0].receive(packets[1]);
  const output=peers.map(peer=>peer.take());assert.deepEqual(output[0],output[1]);
  if(frame>=2){assert.equal(output[0][0][0],(frame-2)%2?256:0);assert.equal(output[0][1][0],512);}
  assert.ok(peers.every(peer=>peer.queues.every(queue=>queue.size<=2)));
}
const stalled=new InputTimeline({localSlot:0,delay:0});stalled.sample(NEUTRAL);
assert.equal(stalled.take(),null);assert.equal(stalled.frame,0);
stalled.receive(encodeInput(0,input));assert.throws(()=>stalled.receive(encodeInput(0,NEUTRAL)));
assert.throws(()=>stalled.receive(encodeInput(121,NEUTRAL)));assert.ok(stalled.take());
stalled.receive(encodeInput(0,NEUTRAL));assert.equal(stalled.queues[1].size,0);
assert.throws(()=>stalled.take());
console.log('PASS 10,000 paired input ticks, exact float32 packets, delayed/duplicate/missing input, immutability and bounded queues');
