addToLibrary({
  melee_browser_net_exchange__proxy:'sync',
  melee_browser_net_exchange__sig:'vpp',
  melee_browser_net_exchange__deps:['melee_browser_poll_input'],
  melee_browser_net_exchange: (output,status) => {
    // All callbacks settle before the waiting C stack can be released. A
    // timeout cancels this request; late results never touch its pointers.
    let settled=false;
    const finish=(pads,error)=>{
      if(settled)return;settled=true;clearTimeout(timer);
      if(error){Module.printErr('Online input: '+error);Atomics.store(HEAP32,status>>2,-1);}
      else {
        HEAPF32.set(pads.flat(),output>>2);Atomics.store(HEAP32,status>>2,1);
      }
      Atomics.notify(HEAP32,status>>2);
    };
    const timer=setTimeout(()=>{
      try{Module.netSession.close('Peer input timed out');}finally{finish(null,'Peer input timed out');}
    },15000);
    _melee_browser_poll_input(output);
    const localPad=Array.from(HEAPF32.subarray(output>>2,(output>>2)+8));
    Promise.resolve().then(()=>Module.netSession.nextInput(localPad)).then(pads=>{
      if(!Array.isArray(pads)||pads.length!==2||pads.some(p=>!Array.isArray(p)||p.length!==8||p.some(x=>!Number.isFinite(x))))
        throw new Error('Invalid synchronized input');
      finish(pads);
    }).catch(error=>finish(null,String(error)));
  },
});
