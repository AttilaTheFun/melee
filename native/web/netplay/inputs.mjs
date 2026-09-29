/** Canonical input samples match browser_input.c's eight-float layout.
 * Simulation must call sample() once and take() once for each logic tick,
 * not each rendered frame. A missing peer sample stalls instead of guessing. */
const MAGIC=0x3149504d; // MPI1, little endian
export const NEUTRAL=Object.freeze([0,0,0,0,0,0,0,1]);
function frameNumber(value){if(!Number.isInteger(value)||value<0||value>0xffffffff)throw new RangeError('Invalid input frame');return value;}
export function normalizePad(pad) {
  if(!pad||pad.length!==8)throw new TypeError('Expected eight input values');
  const p=Array.from(pad,Math.fround);
  if(!p.every(Number.isFinite)||!Number.isInteger(p[0])||p[0]<0||p[0]>65535||
    p.slice(1,5).some(x=>x < -1||x>1)||p.slice(5,7).some(x=>x<0||x>1)||![0,1].includes(p[7]))
    throw new RangeError('Invalid controller sample');
  return Object.freeze(p);
}
export function encodeInput(frame,pad) {
  frameNumber(frame);const p=normalizePad(pad),bytes=new Uint8Array(40),view=new DataView(bytes.buffer);
  view.setUint32(0,MAGIC,true);view.setUint32(4,frame,true);
  for(let i=0;i<8;i++)view.setFloat32(8+i*4,p[i],true);
  return bytes;
}
export function decodeInput(bytes) {
  if(!(bytes instanceof Uint8Array)||bytes.length!==40)throw new TypeError('Invalid input packet length');
  const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
  if(view.getUint32(0,true)!==MAGIC)throw new Error('Incompatible input protocol');
  return {frame:view.getUint32(4,true),pad:normalizePad(Array.from({length:8},(_,i)=>view.getFloat32(8+i*4,true)))};
}
export class InputTimeline {
  constructor({localSlot,delay=2,horizon=120}) {
    if(![0,1].includes(localSlot)||!Number.isInteger(delay)||delay<0||delay>12||!Number.isInteger(horizon)||horizon<delay+1||horizon>600)
      throw new RangeError('Invalid lockstep configuration');
    this.localSlot=localSlot;this.delay=delay;this.horizon=horizon;this.frame=0;this.sampledFrame=-1;
    this.queues=[new Map(),new Map()];
    for(let frame=0;frame<delay;frame++)for(const queue of this.queues)queue.set(frame,NEUTRAL);
  }
  sample(pad) {
    if(this.sampledFrame===this.frame)throw new Error('Input already sampled for this tick');
    const target=frameNumber(this.frame+this.delay),sample=normalizePad(pad);
    this.queues[this.localSlot].set(target,sample);this.sampledFrame=this.frame;
    return encodeInput(target,sample);
  }
  receive(packet) {
    const {frame,pad}=decodeInput(packet);
    if(frame<this.frame)return; // a delayed duplicate cannot change history
    if(frame>this.frame+this.horizon)throw new Error('Peer input exceeds lookahead window');
    const queue=this.queues[1-this.localSlot],old=queue.get(frame);
    if(old&&old.some((value,index)=>value!==pad[index]))throw new Error('Peer changed committed input');
    queue.set(frame,pad);
  }
  take() {
    if(this.sampledFrame!==this.frame)throw new Error('Sample local input before advancing');
    if(!this.queues.every(queue=>queue.has(this.frame)))return null;
    const result=this.queues.map(queue=>{const pad=queue.get(this.frame);queue.delete(this.frame);return pad;});
    this.frame++;return result;
  }
}
