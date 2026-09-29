/** Two actual Wasm game instances over real WebRTC. No listening HTTP server. */
import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE||'playwright');
if(!process.env.MELEE_DISC)throw new Error('MELEE_DISC is required');
const here=path.dirname(fileURLToPath(import.meta.url));
const root=process.env.MELEE_WASM_BUILD?path.resolve(process.env.MELEE_WASM_BUILD):path.resolve(here,'../build/wasm-renderer');
const limit=Number(process.env.MELEE_NET_TICKS||600);
const jitter=!!process.env.MELEE_NET_JITTER;
const bootDelay=Number(process.env.MELEE_NET_BOOT_DELAY||0);
const clockOffset=Number(process.env.MELEE_NET_CLOCK_OFFSET_MS||0);
if(!Number.isFinite(clockOffset))throw new Error('Invalid test clock offset');
const inputDelay=Number(process.env.MELEE_NET_DELAY||2);
const catchupLimits=(process.env.MELEE_NET_CATCHUP||'3,3').split(',').map(Number);
const iceServers=process.env.MELEE_ICE_CONFIG?JSON.parse(await fs.readFile(process.env.MELEE_ICE_CONFIG,'utf8')):[];
const relayOnly=!!process.env.MELEE_RELAY_ONLY;
const build=createHash('sha256').update(await fs.readFile(path.join(root,'melee_browser.wasm'))).digest('hex');
// Both contexts select the same local file; production computes a streaming
// fingerprint. This test token identifies that explicitly shared test source.
const disc=createHash('sha256').update(process.env.MELEE_DISC).digest('hex');
const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_BINARY||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
 args:['--enable-unsafe-webgpu','--use-angle=metal','--mute-audio','--autoplay-policy=no-user-gesture-required']});
let pages=[],shuttingDown=false;
try{
 const contexts=await Promise.all([browser.newContext(),browser.newContext()]);
 for(const [contextSlot,context] of contexts.entries())await context.route('https://melee.test/**',async route=>{
   const name=new URL(route.request().url()).pathname.slice(1);
   const headers={'Cross-Origin-Opener-Policy':'same-origin','Cross-Origin-Embedder-Policy':'require-corp'};
   if(!name)return route.fulfill({headers,contentType:'text/html',body:'<!doctype html><canvas id="canvas" width="640" height="480"></canvas><input id="disc" type="file">'});
   let file;
   if(['melee_browser.js','melee_browser.wasm','audio-worklet.js'].includes(name))file=path.join(root,name);
   else if(['transport.mjs','inputs.mjs','session.mjs'].includes(name))file=path.join(here,'netplay',name);
   else return route.abort();
   let body=await fs.readFile(file);
   // Each pthread loads this script into its own realm. Skew Date.now there
   // too: changing only the page clock does not affect OSGetTime on workers.
   if(contextSlot===1&&clockOffset&&name==='melee_browser.js')body=Buffer.concat([
     Buffer.from(`{const realNow=Date.now;Date.now=()=>realNow()+${clockOffset};}\n`),body]);
   return route.fulfill({headers,contentType:name.endsWith('.wasm')?'application/wasm':'text/javascript',body});
 });
 pages=await Promise.all(contexts.map(c=>c.newPage()));const errors=[];
 for(let slot=0;slot<2;slot++){
   const page=pages[slot];page.on('pageerror',error=>{if(shuttingDown)return;errors.push(String(error));console.error('PEER ERROR',slot,String(error));});
   page.on('console',message=>{if(shuttingDown)return;if(message.text().includes('Online input')||message.text().includes('signature mismatch'))console.log('PEER',slot,message.text());});
   await page.exposeFunction('signal',message=>pages[1-slot].evaluate(message=>peer.acceptSignal(message),message));
   await page.goto('https://melee.test/');
   await page.evaluate(async({slot,build,disc,limit,jitter,iceServers,relayOnly,catchupLimit,inputDelay,bootDelay})=>{
     const {PeerTransport}=await import('/transport.mjs'),{GameSession}=await import('/session.mjs');
     window.historyByTick={};window.failure=null;window.reachedLimit=false;window.matchTiming=null;window.matchEntered=null;window.resultTick=null;
     window.Module={disableSaving:true,netCatchupLimit:catchupLimit,print:()=>{},printErr:message=>console.log(message),onAbort:message=>{window.failure=String(message);},
       onGameState:state=>{
         if(state.netTick)historyByTick[state.netTick]={mode:state.mode,scene:state.scene,rng:state.rng,cursorTargets:state.cursorTargets,stageGuidance:state.stageGuidance,fighters:state.fighters};
         if(state.mode===2&&state.scene===4&&resultTick===null)resultTick=state.netTick;
         if(state.mode===2&&state.scene===2&&state.fighters.every(f=>f[0]===1)){
           if(matchEntered===null)matchEntered=state.netTick;
           if(state.netTick-matchEntered>=300){
             const point=[state.netTick,performance.now(),state.frame];
             if(!matchTiming)matchTiming={first:point,last:point};else matchTiming.last=point;
           }
         }
       }};
     window.peer=new PeerTransport({initiator:slot===0,iceServers,relayOnly,onSignal:signal,onPacket:()=>{},onClose:reason=>{window.failure=reason;}});
     Module.netSession=new GameSession({transport:peer,localSlot:slot,seed:123456,delay:inputDelay,build,disc});
     if(jitter){
       // Delay delivery after the real ordered data channel. Preserve ordering
       // while varying latency asymmetrically; this is not a TURN/network test.
       const receive=peer.onPacket;let deliverAt=0,packetCount=0;
       peer.onPacket=packet=>{
         const now=performance.now();deliverAt=Math.max(deliverAt,now+10+(packetCount++%7)*7+slot*13);
         setTimeout(()=>receive(packet),deliverAt-now);
       };
     }
     const next=Module.netSession.nextInput.bind(Module.netSession);
     let sceneKey=null,entered=0,selectedAt=null;
     Module.netSession.nextInput=pad=>{
       const frame=Module.netSession.timeline.frame;
       const stopAt=limit>=9000?Math.max(limit,(window.resultTick??Infinity)+120):limit;
       if(frame>limit+6000)throw new Error('Full match exceeded its bounded tick extension');
       if(frame>=stopAt){window.reachedLimit=true;return new Promise((resolve,reject)=>{window.stopAtLimit=reject;});}
       const sample=[0,0,0,0,0,0,0,1],state=Module.meleeState;
       const key=state?state.mode+':'+state.scene:null;
       if(key!==sceneKey){sceneKey=key;entered=frame;selectedAt=null;}
       const age=frame-entered;
       if(slot===0){
         if(state?.mode===40&&frame%120<6)sample[0]=256;
         if((state?.mode===24||state?.mode===0)&&frame%60<6)sample[0]=4096;
         if(state?.mode===1){
           if(age>=30&&age<36)sample[2]=-1;
           if(age>=60&&age%60<6)sample[0]=256;
         }
       }
       if(state?.mode===2&&state.scene===0&&age>30){
         const [dx,dy]=state.cursorTargets[slot];
         if(selectedAt===null){
           if(Math.abs(dx)<0.6&&Math.abs(dy)<0.6)selectedAt=frame;
           else if(frame%4===0){sample[1]=Math.abs(dx)>0.6?Math.sign(dx)*0.5:0;sample[2]=Math.abs(dy)>0.6?Math.sign(dy)*0.5:0;}
         }
         if(selectedAt!==null&&frame-selectedAt<4)sample[0]=256;
         if(slot===0&&age>600&&age%60<6)sample[0]=4096;
       }
       if(slot===0&&state?.mode===2&&state.scene===1&&age>30){
         const guidance=state.stageGuidance;
         if(guidance===16&&selectedAt===null)selectedAt=frame;
         if(selectedAt!==null&&frame-selectedAt<4)sample[0]=256;
         else if(selectedAt===null&&frame%4===0){
           sample[1]=(guidance&1)?-0.65:(guidance&2)?0.65:0;
           sample[2]=(guidance&4)?-0.65:(guidance&8)?0.65:0;
         }
       }
       if(state?.mode===2&&state.scene===2){
         if(age>=180&&age<210)sample[1]=slot?-1:1;
         if(age>=240&&age<246)sample[0]=256;
         if(age>=300&&age<306)sample[0]=1024;
       }
       // End sudden death with ordinary input: P1 walks off the left edge.
       // This makes the results regression independent of idle bomb placement.
       if(slot===0&&state?.mode===2&&state.scene===3&&age>=30)sample[1]=-1;
       return next(sample);
     };
     window.startGame=async()=>{
       await Module.netSession.start();
       if(slot===1&&bootDelay)await new Promise(resolve=>setTimeout(resolve,bootDelay));
       Module.discFile=document.querySelector('#disc').files[0];
       Module.audioContext=new AudioContext({sampleRate:32000});await Module.audioContext.resume();
       const url=URL.createObjectURL(new Blob([await (await fetch('/audio-worklet.js')).text()],{type:'text/javascript'}));
       await Module.audioContext.audioWorklet.addModule(url);URL.revokeObjectURL(url);
       const script=document.createElement('script');script.src='/melee_browser.js';document.body.append(script);
     };
   },{slot,build,disc,limit,jitter,iceServers,relayOnly,catchupLimit:catchupLimits[slot],inputDelay,bootDelay});
   await page.locator('#disc').setInputFiles(process.env.MELEE_DISC);
 }
 await pages[0].evaluate(()=>peer.offer());
 await Promise.all(pages.map(page=>page.evaluate(()=>startGame())));
 for(let slot=0;slot<2;slot++){
   const route=await pages[slot].evaluate(()=>peer.route());
   assert.ok(route);if(relayOnly)assert.equal(route.relay,true,'Game used a non-relay path');
   console.log('GAME ROUTE',slot,JSON.stringify(route));
 }
 const started=performance.now(),deadline=started+Math.max(240000,limit*100);
 while(performance.now()<deadline){
   const states=await Promise.all(pages.map(page=>page.evaluate(()=>({done:reachedLimit,failure,phase:Module.netNativePhase?.(),profile:Module.runtimeProfile,state:Module.meleeState}))));
   console.log('NET RUN',JSON.stringify(states.map(state=>({...state,testElapsedMs:performance.now()-started}))));
   if(errors.length||states.some(s=>s.failure))throw new Error('Peer runtime failed');
   if(states.every(s=>s.done))break;
   await new Promise(resolve=>setTimeout(resolve,5000));
 }
 const results=await Promise.all(pages.map(page=>page.evaluate(()=>({done:reachedLimit,history:historyByTick,timing:matchTiming}))));
 await fs.writeFile(path.join(root,'net-game-result.json'),JSON.stringify(results));
 assert.ok(results.every(result=>result.done),'Peers did not reach the requested tick count');
 let checked=0;
 for(const [tick,state] of Object.entries(results[0].history)){
   if(!results[1].history[tick])continue;
   assert.deepEqual(state,results[1].history[tick],'Divergence at logic tick '+tick);checked++;
 }
 assert.ok(checked>=limit-10,'Insufficient matching tick snapshots');
 if(limit>=2000)assert.ok(Object.values(results[0].history).some(state=>state.mode===2&&state.scene===2&&state.fighters.every(f=>f[0]===1)),'Network scenario did not reach a two-player match');
 if(limit>=9000)assert.ok(Object.values(results[0].history).some(state=>state.mode===2&&state.scene===4),'Network match did not reach results');
 for(const [slot,result] of results.entries())if(result.timing){
   const {first,last}=result.timing,seconds=(last[1]-first[1])/1000;
   const ticksPerSecond=(last[0]-first[0])/seconds,rendersPerSecond=(last[2]-first[2])/seconds;
   console.log('GAME PACING',slot,JSON.stringify({seconds,ticksPerSecond,rendersPerSecond}));
   if(process.env.MELEE_NET_MIN_TPS)assert.ok(ticksPerSecond>=Number(process.env.MELEE_NET_MIN_TPS),'Simulation below requested pacing target');
 }
 assert.deepEqual(errors,[],'Unexpected browser errors');
 console.log('PASS two Wasm engines over WebRTC:',checked,'matching logic-tick snapshots');
}finally{
 shuttingDown=true;
 await Promise.allSettled(pages.map(page=>page.evaluate(()=>{
   window.stopAtLimit?.(new Error('Test complete'));
   Module.netSession?.close('Test complete');
 })));
 await browser.close();
}
