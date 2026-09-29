addToLibrary({
  melee_browser_audio_attach__proxy:'sync',
  melee_browser_audio_attach__sig:'vp',
  melee_browser_audio_attach: (layoutPtr) => {
    if(!Module.audioContext || Module.audioNode)return;
    const context=Module.audioContext;
    if(context.sampleRate!==32000){Module.printErr('Audio requires a 32 kHz AudioContext.');return;}
    const layout=Array.from(HEAPU32.subarray(layoutPtr/4,layoutPtr/4+7));
    const node=Module.audioNode=new AudioWorkletNode(context,'melee-pcm',{
      numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[2],processorOptions:{memory:wasmMemory.buffer,layout}});
    node.port.onmessage=event=>{Module.audioStats=event.data;};
    node.onprocessorerror=()=>Module.printErr('The audio worklet stopped. Reload to restart audio.');
    node.connect(context.destination);
  },
});
