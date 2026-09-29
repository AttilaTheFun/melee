import {snapshot} from './scene-data.js';
import createMelee from './melee.mjs';
let m, mounted=false;
const ready=createMelee({print:()=>{},printErr:console.warn}).then(module=>{m=module;m.FS.mkdir('/disc');});
function check(result) { if(!result)throw Error(m.UTF8ToString(m._web_error())); }
self.onmessage=async ({data})=>{
  try {
    await ready;
    if(data.type==='load') {
      if(data.file) {
        // Close descriptors before replacing the read-only File mount.
        m.ccall('web_open_disc','number',['string'],['/missing']);
        if(mounted)m.FS.unmount('/disc');
        m.FS.mount(m.WORKERFS,{blobs:[{name:'game.iso',data:data.file}]},'/disc');mounted=true;
        check(m.ccall('web_open_disc','number',['string'],['/disc/game.iso']));
      }
      check(m._web_load_fighter(data.fighter));
      const {parts,transfers}=snapshot(m,true);
      self.postMessage({id:data.id,type:'loaded',parts,duration:m._web_duration(),omitted:m._web_omitted_layers()},transfers);
    } else if(data.type==='step') {
      check(m._web_step(data.frame));const {parts,transfers}=snapshot(m,false);
      self.postMessage({id:data.id,type:'frame',parts},transfers);
    }
  } catch(error) {self.postMessage({id:data.id,type:'error',message:String(error.message||error)});}
};
