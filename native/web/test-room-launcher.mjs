/** Launcher + real WebRTC integration; signaling boundary is mocked, no listener. */
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE||'playwright');
const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_BINARY||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',args:['--enable-unsafe-webgpu','--autoplay-policy=no-user-gesture-required']});
const fullGame=process.env.MELEE_LAUNCHER_GAME==='1';
if(fullGame&&!process.env.MELEE_DISC)throw new Error('Full launcher test requires MELEE_DISC');
const room={id:'a'.repeat(64),hostToken:'b'.repeat(64),joinToken:'c'.repeat(64)};
const sockets=[];const errors=[];let chosenDelay=3;
try{
 const pages=[];
 for(let slot=0;slot<2;slot++){
  const context=await browser.newContext();
  await context.route('https://melee.test/**',async route=>{
   const path=new URL(route.request().url()).pathname;
   const headers={'Cross-Origin-Opener-Policy':'same-origin','Cross-Origin-Embedder-Policy':'require-corp'};
   let body,contentType='application/json';
   if(path==='/api/rooms'){chosenDelay=JSON.parse(route.request().postData()).delay;body=JSON.stringify(room);}
   else if(path.endsWith('/ice'))body=JSON.stringify({iceServers:[]});
   else if(fullGame&&['/build.json','/melee_browser.js','/melee_browser.wasm'].includes(path)){
    body=await fs.readFile(new URL('../build/wasm-site'+path,import.meta.url));
    contentType=path.endsWith('.wasm')?'application/wasm':path.endsWith('.js')?'text/javascript':'application/json';
   }
   else if(path==='/build.json')body=JSON.stringify({build:'d'.repeat(64)});
   else if(path==='/melee_browser.js'){body='window.gameBooted=true;';contentType='text/javascript';}
   else{
    const relative=path==='/'?'game/index.html':path.startsWith('/netplay/')?path.slice(1):'game'+path;
    body=await fs.readFile(new URL('./'+relative,import.meta.url));
    contentType=path==='/'?'text/html':'text/javascript';
   }
   await route.fulfill({body,contentType,headers});
  });
  await context.routeWebSocket('wss://melee.test/**',ws=>{
   sockets[slot]=ws;
   ws.onMessage(data=>sockets[1-slot]?.send(JSON.stringify({type:'signal',signal:JSON.parse(data)})));
   ws.send(JSON.stringify({type:'welcome',slot,seed:123,delay:chosenDelay,expiresAt:Date.now()+3600000}));
   if(sockets.filter(Boolean).length===2)for(const peer of sockets)peer.send(JSON.stringify({type:'peer-ready'}));
  });
  const page=await context.newPage();page.on('pageerror',e=>errors.push(String(e)));await page.goto('https://melee.test/');pages.push(page);
  if(fullGame){
   await page.evaluate(()=>{
    window.gameStates={};
    Module.onGameState=state=>{if(state.netTick<=480)gameStates[state.netTick]=state;};
    const abort=Module.onAbort;Module.onAbort=message=>{window.gameFailure=String(message);abort(message);};
   });
   await page.locator('#disc').setInputFiles(process.env.MELEE_DISC);
  }else await page.locator('#disc').setInputFiles({name:'fixture.ciso',mimeType:'application/octet-stream',buffer:Buffer.from('matching test file')});
 }
 await pages[0].selectOption('#mode','host');await pages[0].selectOption('#input-delay','4');await pages[0].click('#launch');
 await pages[0].waitForFunction(()=>document.querySelector('#invite-link').value.length>0,undefined,{timeout:120000});
 const link=await pages[0].inputValue('#invite-link');assert.ok(link.includes(room.joinToken));assert.ok(!link.includes(room.hostToken));
 await pages[1].selectOption('#mode','join');await pages[1].fill('#invitation',link);await pages[1].click('#launch');
 await Promise.all(pages.map(p=>p.waitForFunction(full=>full?
   (window.gameFailure||Module.meleeState?.netTick>=480):window.gameBooted,fullGame,{timeout:fullGame?180000:30000})));
 const states=await Promise.all(pages.map(p=>p.evaluate(()=>({slot:Module.netSession.timeline.localSlot,saving:Module.disableSaving,hello:Module.netSession.peerHello,delay:Module.netSession.identity.delay}))));
 assert.deepEqual(states,[{slot:0,saving:true,hello:true,delay:4},{slot:1,saving:true,hello:true,delay:4}]);
 if(fullGame){
  const results=await Promise.all(pages.map(p=>p.evaluate(()=>({states:gameStates,failure:window.gameFailure,
    closed:Module.netSession.closed,status:document.querySelector('#status').textContent,pauseDisabled:document.querySelector('#pause').disabled}))));
  for(const result of results){assert.equal(result.failure,undefined);assert.equal(result.closed,false);assert.equal(result.pauseDisabled,true);}
  for(let tick=1;tick<=480;tick++){
   assert.ok(results[0].states[tick],'Missing host tick '+tick);assert.ok(results[1].states[tick],'Missing joiner tick '+tick);
   assert.deepEqual(results[0].states[tick],results[1].states[tick],'Launcher game mismatch at '+tick);
  }
  await fs.writeFile(new URL('../build/launcher-game-result.json',import.meta.url),JSON.stringify(results,null,2));
  console.log('PASS actual launcher → local disc fingerprint → WebRTC → two Wasm runtimes: 480 matching ticks');
 }else{
  const pads=await Promise.all(pages.map(p=>p.evaluate(()=>Module.netSession.nextInput([0,0,0,0,0,0,0,1]))));assert.deepEqual(pads[0],pads[1]);
 }
 assert.deepEqual(errors,[]);
 console.log('PASS host invitation → join → real WebRTC handshake → matching inputs; fresh saves and distinct slots');
}finally{await browser.close();}
