import {InputTimeline} from './inputs.mjs';
const encoder=new TextEncoder(),decoder=new TextDecoder('utf-8',{fatal:true});
const HELLO=[77,72,74,49];
export class GameSession {
  constructor({transport,localSlot,seed,delay=2,build,disc}) {
    if(!Number.isInteger(seed)||seed<0||seed>0xffffffff||!/^[a-f0-9]{64}$/.test(build)||!/^[a-f0-9]{64}$/.test(disc))
      throw new TypeError('Invalid game session identity');
    this.transport=transport;this.seed=seed;this.timeline=new InputTimeline({localSlot,delay});
    this.identity={protocol:1,seed,delay,build,disc};this.closed=false;this.started=false;this.localChecks=new Map();this.remoteChecks=new Map();this.lastVerified=0;
    this.hello=new Promise((resolve,reject)=>{this.resolveHello=resolve;this.rejectHello=reject;});this.hello.catch(()=>{});
    transport.onPacket=packet=>this.receive(packet);
    const previousClose=transport.onClose;
    transport.onClose=reason=>{this.close(reason);previousClose(reason);};
  }
  async start() {
    if(this.started)throw new Error('Session already started');this.started=true;
    const timer=setTimeout(()=>this.close('Session handshake timed out'),15000);
    try {
      await this.transport.ready;
      const body=encoder.encode(JSON.stringify({...this.identity,slot:this.timeline.localSlot})),packet=new Uint8Array(body.length+4);
      packet.set(HELLO);packet.set(body,4);this.transport.send(packet);
      await this.hello;
    }finally{clearTimeout(timer);}
  }
  receive(packet) {
    if(this.closed)return;
    try {
      if(HELLO.every((value,index)=>packet[index]===value)){
        if(this.peerHello||packet.length>1024)throw new Error('Invalid session handshake');
        const identity=JSON.parse(decoder.decode(packet.subarray(4)));
        if(identity.slot!==1-this.timeline.localSlot)throw new Error('Peers selected the same player slot');
        for(const [key,value] of Object.entries(this.identity))if(identity[key]!==value)
          throw new Error('Peer session differs: '+key);
        this.peerHello=true;this.resolveHello();return;
      }
      if(!this.peerHello)throw new Error('Input arrived before session handshake');
      if(packet.length===12&&packet[0]===77&&packet[1]===72&&packet[2]===67&&packet[3]===49){
        const view=new DataView(packet.buffer,packet.byteOffset,packet.byteLength),tick=view.getUint32(4,true),hash=view.getUint32(8,true);
        if(tick%60||tick>this.timeline.frame+120)throw new Error('Invalid state-check tick');
        if(tick<=this.lastVerified)return;
        const old=this.remoteChecks.get(tick);if(old!==undefined&&old!==hash)throw new Error('Peer changed state checksum');
        this.remoteChecks.set(tick,hash);if(this.remoteChecks.size>8)throw new Error('Too many unverified state checks');
        this.compareState(tick);return;
      }
      this.timeline.receive(packet);this.deliver();
    }catch(error){this.close(error.message);}
  }
  nextInput(localPad) {
    if(this.closed)return Promise.reject(new Error(this.reason));
    if(!this.started||!this.peerHello)return Promise.reject(new Error('Session handshake incomplete'));
    if(this.pending)return Promise.reject(new Error('Concurrent simulation ticks'));
    return new Promise((resolve,reject)=>{
      this.pending={resolve,reject};
      try{this.transport.send(this.timeline.sample(localPad));this.deliver();}
      catch(error){this.close(error.message);}
    });
  }
  deliver() {
    if(!this.pending)return;
    const pads=this.timeline.take();
    if(pads){const pending=this.pending;this.pending=null;pending.resolve(pads);}
  }
  observeState(state) {
    const tick=state.netTick;
    if(this.closed||!tick||tick%60||tick<=this.lastVerified||this.localChecks.has(tick))return;
    // A compact simulation signature, not a serialization for rollback. No
    // pointers, wall-clock values or renderer-specific state enter the hash.
    const values=[state.mode,state.scene,state.rng,state.stageGuidance,...state.cursorTargets.flat(),...state.fighters.flat()];
    let hash=2166136261;
    for(const byte of encoder.encode(JSON.stringify(values)))hash=Math.imul(hash^byte,16777619)>>>0;
    this.localChecks.set(tick,hash);
    const packet=new Uint8Array(12),view=new DataView(packet.buffer);
    packet.set([77,72,67,49]);view.setUint32(4,tick,true);view.setUint32(8,hash,true);
    try {
      this.transport.send(packet);this.compareState(tick);
      if(tick-this.lastVerified>180)throw new Error('Peer stopped sending state checks');
    }catch(error){this.close(error.message);}
  }
  compareState(tick) {
    for(let next=this.lastVerified+60;this.localChecks.has(next)&&this.remoteChecks.has(next);next+=60){
      if(this.localChecks.get(next)!==this.remoteChecks.get(next))throw new Error('Game desynchronized at tick '+next);
      this.localChecks.delete(next);this.remoteChecks.delete(next);this.lastVerified=next;
    }
  }
  close(reason='Disconnected') {
    if(this.closed)return;this.closed=true;this.reason=reason;
    this.rejectHello(new Error(reason));this.pending?.reject(new Error(reason));this.pending=null;
    this.transport.close(reason);
  }
}
/** Bounded-memory fingerprint of an exact disc file. Different ISO/CISO file
 * representations deliberately do not compare equal in this initial protocol. */
export async function fingerprintFile(file,onProgress=()=>{}) {
  const blockSize=8*1024*1024,hashes=[];
  for(let offset=0;offset<file.size;offset+=blockSize){
    const bytes=await file.slice(offset,offset+blockSize).arrayBuffer();
    hashes.push(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)));
    onProgress(Math.min(offset+blockSize,file.size)/file.size);
  }
  const manifest=new Uint8Array(8+hashes.length*32);
  new DataView(manifest.buffer).setBigUint64(0,BigInt(file.size),true);
  hashes.forEach((hash,index)=>manifest.set(hash,8+index*32));
  return Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',manifest)),x=>x.toString(16).padStart(2,'0')).join('');
}
