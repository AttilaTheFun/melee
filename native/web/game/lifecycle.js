addToLibrary({
  melee_browser_lifecycle_attach__proxy:'sync',
  melee_browser_lifecycle_attach__sig:'vp',
  melee_browser_lifecycle_attach: (pointer) => {
    let transition=Promise.resolve();
    Module.setPaused=paused=>{
      // Serialize audio transitions and flag publication so rapid clicks cannot
      // resume the simulation while a prior suspend is still pending.
      transition=transition.catch(()=>{}).then(async()=>{
        if(paused){
          Atomics.store(HEAP32,pointer>>2,1);
          if(Module.audioContext?.state==='running')await Module.audioContext.suspend();
        }else{
          if(Module.audioContext?.state==='suspended')await Module.audioContext.resume();
          Atomics.store(HEAP32,pointer>>2,0);Atomics.notify(HEAP32,pointer>>2);
        }
      });
      return transition;
    };
    document.addEventListener('visibilitychange',()=>{
      if(document.hidden)Module.setPaused(true).catch(error=>Module.printErr('Pause failed: '+error));
    });
    Module.onLifecycleReady?.();
  },
  melee_browser_lifecycle_changed__proxy:'sync',
  melee_browser_lifecycle_changed__sig:'vi',
  melee_browser_lifecycle_changed: (paused) => {
    Module.gamePaused=!!paused;Module.onPauseChanged?.(!!paused);
  },
});
