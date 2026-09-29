import {Renderer} from './renderer.js';
const $=id=>document.getElementById(id),canvas=$('canvas');
let renderer,worker,busy=false,loaded=false,playing=true,frame=0,duration=60,last=0,nextId=0,activeId=0;
function status(message,error=false){$('status').textContent=message;$('status').classList.toggle('error',error);}
function controls(enabled){$('fighter').disabled=!enabled;$('play').disabled=!enabled;$('frame').disabled=!enabled;}
function fail(error){loaded=false;busy=false;controls(false);$('disc').disabled=false;status(error.message||String(error),true);}
function request(type,extra={}) {busy=true;activeId=++nextId;worker.postMessage({id:activeId,type,...extra});}
async function load(file){loaded=false;controls(false);$('disc').disabled=true;frame=0;status('Reading character assets and compiling WebGPU materials…');while(busy)await new Promise(resolve=>setTimeout(resolve,10));request('load',{file,fighter:Number($('fighter').value)});}
$('disc').addEventListener('change',()=>{const file=$('disc').files[0];if(file&&worker)load(file);});
$('fighter').addEventListener('change',()=>load());
$('play').addEventListener('click',()=>{playing=!playing;$('play').textContent=playing?'Pause':'Play';});
$('frame').addEventListener('input',()=>{frame=Number($('frame').value);playing=false;$('play').textContent='Play';});
$('reset').addEventListener('click',()=>renderer?.reset());
let drag;
canvas.addEventListener('pointerdown',e=>{drag=[e.clientX,e.clientY];canvas.setPointerCapture(e.pointerId);});
canvas.addEventListener('pointerup',()=>drag=null);canvas.addEventListener('pointercancel',()=>drag=null);
canvas.addEventListener('pointermove',e=>{if(drag&&renderer){renderer.yaw-=(e.clientX-drag[0])*.008;renderer.pitch=Math.max(-1.3,Math.min(1.3,renderer.pitch+(e.clientY-drag[1])*.008));drag=[e.clientX,e.clientY];}});
canvas.addEventListener('wheel',e=>{e.preventDefault();if(renderer)renderer.distance=Math.max(12,Math.min(100,renderer.distance*Math.exp(e.deltaY*.001)));},{passive:false});
function tick(now){
 const dt=last?Math.min((now-last)/1000,.1):0;last=now;
 if(loaded){if(playing&&!document.hidden)frame=(frame+dt*60)%duration;$('frame').value=frame;$('frameLabel').textContent=frame.toFixed(1);if(!busy)request('step',{frame});}
 try{renderer?.draw();}catch(e){fail(e);return;}
 requestAnimationFrame(tick);
}
try {
 $('disc').disabled=true;
 renderer=await Renderer.create(canvas,fail);
 worker=new Worker(new URL('./worker.js',import.meta.url),{type:'module'});
 worker.onerror=e=>fail(Error(e.message||'The Wasm worker failed. Reload to retry.'));
 worker.onmessage=async({data})=>{
  if(data.id!==activeId)return;
  try {
   if(data.type==='error')throw Error(data.message);
   if(data.type==='loaded'){
    await renderer.load(data.parts);duration=data.duration;$('frame').max=Math.max(0,duration-.01);
    const vertices=data.parts.reduce((n,p)=>n+p.vertices.length/12,0),triangles=data.parts.reduce((n,p)=>n+p.indices.length/3,0);
    $('metrics').textContent=`${data.parts.length} geometry parts\n${vertices.toLocaleString()} vertices · ${triangles.toLocaleString()} triangles\n${duration} animation frames\n${data.omitted} texture layers omitted`;
    loaded=true;playing=true;controls(true);$('play').textContent='Pause';$('disc').disabled=false;
    status('Running native animation in Wasm. WebGPU is presenting the model.');
   }else if(data.type==='frame')renderer.update(data.parts);
   busy=false;
  }catch(e){fail(e);}
 };
 $('disc').disabled=false;status('Ready. Choose a local Melee disc to begin.');requestAnimationFrame(tick);
}catch(e){fail(e);$('disc').disabled=true;}
