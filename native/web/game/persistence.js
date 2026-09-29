// IDBFS operations happen on browser main before the game pthread is started.
Module.preRun=Module.preRun||[];
Module.preRun.push(function(){
  FS.mkdir('/save');
  if(Module.disableSaving)return;
  addRunDependency('memory-card');
  if(!navigator.locks){abort('This browser cannot lock save storage. Retry with saving disabled.');return;}
  navigator.locks.request('melee-memory-card',{ifAvailable:true},lock=>{
    if(!lock){abort('Another tab is using this memory card. Close it or disable saving.');return;}
    return new Promise(()=>{
      FS.mount(IDBFS,{autoPersist:true},'/save');
      FS.syncfs(true,error=>{
    if(error){abort('Browser save storage could not be opened. Retry with saving disabled.');return;}
    // An abandoned import must not unexpectedly replace a card on next boot.
    try{FS.unlink('/save/import.card');}catch{}
    if(Module.saveImportBytes)FS.writeFile('/save/import.card',Module.saveImportBytes);
    removeRunDependency('memory-card');
      });
    });
  }).catch(error=>abort('Save storage lock failed: '+error));
});
Module.exportMemoryCard=()=>new Promise((resolve,reject)=>{
  if(Module.disableSaving){reject(new Error('Saving is disabled.'));return;}
  FS.syncfs(false,error=>{
    if(error){reject(error);return;}
    try{resolve(new Blob([FS.readFile('/save/card')],{type:'application/octet-stream'}));}catch(error){reject(error);}
  });
});
