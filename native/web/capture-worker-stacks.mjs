/** Failure-only CDP capture. Uses the existing headless browser pipe; no port. */
export async function captureWorkerStacks(browser) {
  const cdp=await browser.newBrowserCDPSession(),pending=new Map(),paused=new Map();
  let serial=0;
  const receive=({sessionId,message})=>{
    const value=JSON.parse(message);
    if(value.id){const key=sessionId+':'+value.id,request=pending.get(key);
      if(request){pending.delete(key);clearTimeout(request.timer);value.error?request.reject(new Error(value.error.message)):request.resolve(value.result);}}
    else if(value.method==='Debugger.paused')paused.get(sessionId)?.(value.params);
  };
  cdp.on('Target.receivedMessageFromTarget',receive);
  const send=(sessionId,method,params={})=>new Promise((resolve,reject)=>{
    const id=++serial,key=sessionId+':'+id;
    const timer=setTimeout(()=>{pending.delete(key);reject(new Error(method+' timed out'));},5000);
    pending.set(key,{resolve,reject,timer});
    cdp.send('Target.sendMessageToTarget',{sessionId,message:JSON.stringify({id,method,params})})
      .catch(error=>{const request=pending.get(key);if(request){pending.delete(key);clearTimeout(timer);reject(error);}});
  });
  try {
    const {targetInfos}=await cdp.send('Target.getTargets');
    return await Promise.all(targetInfos.filter(target=>target.type==='worker').map(async target=>{
      let sessionId;
      const result={target:target.title,url:target.url};
      try {
        ({sessionId}=await cdp.send('Target.attachToTarget',{targetId:target.targetId,flatten:false}));
        await send(sessionId,'Debugger.enable');
        let timer;
        const stopped=new Promise((resolve,reject)=>{
          timer=setTimeout(()=>reject(new Error('Worker pause timed out')),5000);
          paused.set(sessionId,event=>{clearTimeout(timer);resolve(event);});
        });
        // Register the event waiter before asking V8 to interrupt Wasm.
        stopped.catch(()=>{});
        try {
          await send(sessionId,'Debugger.pause');
          const event=await stopped;
          result.reason=event.reason;
          result.frames=event.callFrames.slice(0,32).map(frame=>({name:frame.functionName,url:frame.url,location:frame.location}));
        } finally {clearTimeout(timer);paused.delete(sessionId);}
      } catch(error){result.error=String(error);}
      finally {
        if(sessionId){
          await send(sessionId,'Debugger.resume').catch(()=>{});
          await cdp.send('Target.detachFromTarget',{sessionId}).catch(()=>{});
        }
      }
      return result;
    }));
  } finally {
    for(const request of pending.values()){clearTimeout(request.timer);request.reject(new Error('Capture closed'));}
    pending.clear();cdp.off('Target.receivedMessageFromTarget',receive);await cdp.detach();
  }
}
