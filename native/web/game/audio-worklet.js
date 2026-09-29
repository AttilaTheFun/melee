// Sole consumer of the C SPSC PCM ring. No messages or main-thread timers in
// the render path. The AudioContext runs at the game's 32 kHz sample rate.
class MeleePCM extends AudioWorkletProcessor {
  constructor(options){
    super();
    const {memory,layout}=options.processorOptions;
    this.words=new Uint32Array(memory);this.wide=new BigUint64Array(memory);
    this.samples=new Int16Array(memory,layout[2],8192*2);
    this.produced=layout[0]/4;this.consumed=layout[1]/4;
    this.requested=layout[3]/8;this.missing=layout[4]/8;this.underruns=layout[5]/8;this.largest=layout[6]/4;
    this.started=false;this.blocks=0;this.peak=0;
  }
  process(inputs,outputs){
    const channels=outputs[0];if(!channels?.length)return true;
    const count=channels[0].length;
    const consumed=Atomics.load(this.words,this.consumed);
    const available=(Atomics.load(this.words,this.produced)-consumed)>>>0;
    if(!this.started){if(available<1600)return true;this.started=true;}
    const take=Math.min(count,available);
    for(let i=0;i<take;i++){
      const index=((consumed+i)%8192)*2;
      const left=this.samples[index]/32768;const right=this.samples[index+1]/32768;
      channels[0][i]=left;if(channels[1])channels[1][i]=right;
      this.peak=Math.max(this.peak,Math.abs(left),Math.abs(right));
    }
    Atomics.store(this.words,this.consumed,(consumed+take)>>>0);
    Atomics.add(this.wide,this.requested,BigInt(count));
    Atomics.store(this.words,this.largest,Math.max(count,Atomics.load(this.words,this.largest)));
    if(take<count){Atomics.add(this.wide,this.missing,BigInt(count-take));Atomics.add(this.wide,this.underruns,1n);this.started=false;}
    if(++this.blocks%250===0)this.port.postMessage({peak:this.peak,frames:Number(Atomics.load(this.wide,this.requested)),underruns:Number(Atomics.load(this.wide,this.underruns))});
    return true;
  }
}
registerProcessor('melee-pcm',MeleePCM);
