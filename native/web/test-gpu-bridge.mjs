/** Actual headless browser test. No host window, input events, or listener. */
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import {captureWorkerStacks} from './capture-worker-stacks.mjs';
const { chromium } = await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const probe = process.env.MELEE_GPU_PROBE || 'melee_gpu_bridge_probe';
if (!/^melee_[a-z_]+$/.test(probe)) throw new Error('Invalid probe name');
const game = probe === 'melee_browser';
if (game && !process.env.MELEE_DISC) throw new Error('Set MELEE_DISC to a local ISO/CISO');
const root = process.env.MELEE_WASM_BUILD ? path.resolve(process.env.MELEE_WASM_BUILD) :
  path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../build/wasm-renderer');
const browser = await chromium.launch({
  executablePath: process.env.CHROME_BINARY || '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  headless: true,
  args: ['--enable-unsafe-webgpu', '--use-angle=metal','--mute-audio','--autoplay-policy=no-user-gesture-required',...(process.env.MELEE_BASELINE?['--js-flags=--liftoff-only']:[])],
});
try {
  const context = await browser.newContext();
  const headers = {
    'Cross-Origin-Opener-Policy': 'same-origin',
    'Cross-Origin-Embedder-Policy': 'require-corp',
    'Cache-Control': 'no-store',
  };
  await context.route('https://melee.test/**', async route => {
    const name = new URL(route.request().url()).pathname.slice(1);
    if (!name) return route.fulfill({ headers, contentType:'text/html', body:`<!doctype html>
      <canvas id="canvas" width="640" height="480"></canvas>
      <script>window.probeMessages=[];window.probeExit=null;var Module={debugGPU:${!!process.env.MELEE_GPU_TRACE},
        print:s=>{probeMessages.push(s);console.log(s)},
        printErr:s=>{probeMessages.push(s);console.error(s)},
        disableSaving:${!process.env.MELEE_TEST_SAVING},onExit:code=>{window.probeExit=code},onAbort:s=>{window.probeAbort=String(s)}};</script>
      ${game ? `<input type="file" id="disc"><script>
        document.querySelector('#disc').onchange=async e=>{Module.discFile=e.target.files[0];
          Module.audioContext=new AudioContext({sampleRate:32000});const worklet=URL.createObjectURL(new Blob([await (await fetch('/audio-worklet.js')).text()],{type:'text/javascript'}));await Module.audioContext.audioWorklet.addModule(worklet);URL.revokeObjectURL(worklet);await Module.audioContext.resume();
          const script=document.createElement('script');script.src='/${probe}.js';document.body.append(script);};
      </script>` : `<script src="/${probe}.js"></script>`}` });
    if (![`${probe}.js`,`${probe}.wasm`,`${probe}.wasm.map`,'audio-worklet.js'].includes(name)) return route.abort();
    await route.fulfill({ headers, contentType:name.endsWith('.wasm')?'application/wasm':name.endsWith('.map')?'application/json':'text/javascript',
      body:await fs.readFile(path.join(root,name)) });
  });
  const page = await context.newPage();
  const pageErrors=[];
  page.on('console', message => {
    const value=message.text();console.log(message.type(),value);
    if(value.includes('ERROR: AddressSanitizer:')){
      pageErrors.push(value);
      page.evaluate(message=>{window.probeAbort=message;},value).catch(()=>{});
    }
  });
  page.on('pageerror', error => {pageErrors.push(String(error));console.error('PAGE ERROR',error);page.evaluate(message=>{window.probeAbort=message;},String(error)).catch(()=>{});});
  await page.goto('https://melee.test/');
  console.log('Browser',await browser.version(),await page.evaluate(()=>({
    isolated:crossOriginIsolated,webgpu:!!navigator.gpu,threads:navigator.hardwareConcurrency})));
  if(game) await page.locator('#disc').setInputFiles(process.env.MELEE_DISC);
  await page.waitForFunction(game=>window.probeExit!==null || window.probeAbort ||
    (game && probeMessages.some(x=>x.includes('Browser game frame 120'))), game, {timeout:game?180000:60000});
  if(game && process.env.MELEE_TEST_PERSISTENCE) {
    if(!process.env.MELEE_TEST_SAVING)throw new Error('Persistence test requires MELEE_TEST_SAVING=1');
    const exportCard=()=>page.evaluate(async()=>Array.from(new Uint8Array(await (await Module.exportMemoryCard()).arrayBuffer())));
    const original=await exportCard();
    if(original.length<8148 || String.fromCharCode(...original.slice(0,6))!=='MLCARD')
      throw new Error('Invalid exported memory card');
    const other=await context.newPage();
    await other.goto('https://melee.test/');
    await other.locator('#disc').setInputFiles(process.env.MELEE_DISC);
    await other.waitForFunction(()=>window.probeAbort,undefined,{timeout:60000});
    if(!await other.evaluate(()=>window.probeAbort.includes('Another tab')))
      throw new Error('Second tab did not reject concurrent card ownership');
    await other.close();
    const reloadCard=async bytes=>{
      await page.reload();
      if(bytes)await page.evaluate(value=>{Module.saveImportBytes=new Uint8Array(value);},bytes);
      await page.locator('#disc').setInputFiles(process.env.MELEE_DISC);
      await page.waitForFunction(()=>window.probeAbort || Module.meleePresentation?.frames>=120,undefined,{timeout:180000});
      if(await page.evaluate(()=>window.probeAbort))throw new Error('Card reload/import failed');
    };
    await reloadCard();
    if(JSON.stringify(await exportCard())!==JSON.stringify(original))throw new Error('Card changed across browser reload');
    // Import a valid nonempty native card with a private test file. Checking
    // payload bytes catches a persistence path that only preserves headers.
    const populated=Buffer.alloc(8148+8192);Buffer.from(original.slice(0,20)).copy(populated);
    populated.write('GALE01',20,'ascii');populated.write('BROWSER-ROUNDTRIP',28,'ascii');
    populated.writeUInt32BE(42,60);populated.writeUInt32BE(0xffffffff,64);
    populated.writeUInt16BE(1,76);populated.writeUInt32BE(0xffffffff,80);
    for(let i=8148;i<populated.length;i++)populated[i]=(i*37+11)&255;
    let crc=0xffffffff;
    for(const byte of populated.subarray(20)){crc^=byte;for(let bit=0;bit<8;bit++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}
    populated.writeUInt32BE((crc^0xffffffff)>>>0,16);
    await reloadCard(Array.from(populated));
    if(!Buffer.from(await exportCard()).equals(populated))throw new Error('Nonempty card import/export mismatch');
    await reloadCard();
    if(!Buffer.from(await exportCard()).equals(populated))throw new Error('Nonempty card lost across reload');
    const corrupted=Array.from(populated);corrupted[corrupted.length-1]^=1;
    const expectedErrorStart=pageErrors.length;
    await page.reload();
    await page.evaluate(value=>{Module.saveImportBytes=new Uint8Array(value);},corrupted);
    await page.locator('#disc').setInputFiles(process.env.MELEE_DISC);
    await page.waitForFunction(()=>window.probeAbort,undefined,{timeout:60000});
    if(!await page.evaluate(()=>probeMessages.some(message=>message.includes('open memory card'))))
      throw new Error('Corrupt import did not fail card validation');
    if(!Buffer.from(await exportCard()).equals(populated))throw new Error('Corrupt import replaced the existing save');
    pageErrors.splice(expectedErrorStart);
    await reloadCard();
    if(!Buffer.from(await exportCard()).equals(populated))throw new Error('Card lost after rejecting corrupt import');
    console.log('PASS browser nonempty memory card reload/import/export, corrupt-import preservation and exclusive tab ownership',populated.length);
  }
  if(game && process.env.MELEE_INTERACTIVE){
    const {createInterface}=await import('node:readline');
    console.log('READY browser input; JSON commands: keys, hold, wait, capture; quit closes browser');
    for await(const line of createInterface({input:process.stdin})){
      const action=JSON.parse(line);if(action.quit)break;
      const keys=action.keys||[];
      for(const key of keys)await page.keyboard.down(key);
      await page.waitForTimeout(action.hold||100);
      for(const key of keys)await page.keyboard.up(key);
      await page.waitForTimeout(action.wait||1000);
      if(action.capture)await page.locator('#canvas').screenshot({path:path.join(root,'browser-interactive.png')});
      console.log('INPUT RESULT',JSON.stringify(await page.evaluate(()=>({audio:Module.audioStats,frame:Module.meleePresentation?.frames,state:Module.meleeState,abort:window.probeAbort}))));
      if(pageErrors.length)throw new Error(pageErrors.join('\n'));
    }
  }
  if(game && process.env.MELEE_NAVIGATE) {
    const steps=process.env.MELEE_INPUT_FILE?JSON.parse(await fs.readFile(process.env.MELEE_INPUT_FILE,'utf8')):process.env.MELEE_INPUT_STEPS?JSON.parse(process.env.MELEE_INPUT_STEPS):['KeyJ','KeyJ','Enter','Enter','KeyS','KeyJ','KeyJ'];
    for(let step=0;step<steps.length;step++){
      const action=typeof steps[step]==='string'?{key:steps[step]}:steps[step];
      if(action.cursorTarget){
        if(action.cursorTarget!=='pikachu')throw new Error('Unknown character target');
        // Icon positions change with unlocked characters; use live read-only deltas.
        // Drive the normal Gamepad API, never write character-selection state.
        await page.evaluate(()=>{
          window.selectionPadDescriptor=Object.getOwnPropertyDescriptor(navigator,'getGamepads');
          window.selectionPads=[0,1].map(index=>({index,id:'Test selection controller',connected:true,mapping:'standard',
            axes:[0,0,0,0],buttons:Array.from({length:16},()=>({pressed:false,value:0}))}));
          Object.defineProperty(navigator,'getGamepads',{configurable:true,value:()=>selectionPads});
        });
        try {
          const selected=[false,false],deadline=performance.now()+45000;
          while(!selected.every(Boolean)&&performance.now()<deadline){
            const state=await page.evaluate(()=>Module.meleeState);
            if(state?.mode!==2||state.scene!==0)throw new Error('Character guidance requires versus selection');
            const samples=state.pikachuCursorTargets.map(([dx,dy],slot)=>{
              if(selected[slot])return [0,0,false];
              if(Math.abs(dx)<0.6&&Math.abs(dy)<0.6){selected[slot]=true;return [0,0,true];}
              return [Math.abs(dx)>0.6?Math.sign(dx)*0.4:0,Math.abs(dy)>0.6?-Math.sign(dy)*0.4:0,false];
            });
            await page.evaluate(samples=>samples.forEach(([x,y,a],slot)=>{
              selectionPads[slot].axes[0]=x;selectionPads[slot].axes[1]=y;
              selectionPads[slot].buttons[0]={pressed:a,value:a?1:0};
            }),samples);
            await page.waitForTimeout(samples.some(sample=>sample[2])?100:40);
            await page.evaluate(()=>selectionPads.forEach(pad=>{pad.axes.fill(0);pad.buttons[0]={pressed:false,value:0};}));
            await page.waitForTimeout(120);
          }
          if(!selected.every(Boolean))throw new Error('Character cursor guidance timed out');
          await page.waitForTimeout(1000);
        } finally {
          await page.evaluate(()=>{
            if(selectionPadDescriptor)Object.defineProperty(navigator,'getGamepads',selectionPadDescriptor);
            else delete navigator.getGamepads;
            delete window.selectionPads;delete window.selectionPadDescriptor;
          });
        }
        console.log('PASS guided character selection',action.cursorTarget);
        continue;
      }
      if(action.stageTarget){
        await selectStage(action.stageTarget);
        console.log('PASS guided stage selection',action.stageTarget);
        await page.locator('#canvas').screenshot({path:path.join(root,'browser-stage-'+action.stageTarget+'.png')});
        continue;
      }
      const keys=action.keys||[action.key];
      const reached=()=>page.evaluate(target=>{
        const state=Module.meleeState;return state && Object.entries(target).every(([key,value])=>state[key]===value);
      },action.until);
      const deadline=Date.now()+30000;
      do {
        if(action.until && await reached())break;
        for(const key of keys)await page.keyboard.down(key);
        await page.waitForTimeout(action.hold||100);
        for(const key of keys)await page.keyboard.up(key);
        await page.waitForTimeout(action.until?500:(action.wait||1800));
        if(!action.until || pageErrors.length)break;
      } while(Date.now()<deadline);
      if(action.until && !pageErrors.length){
        if(!await reached())throw new Error('Navigation did not reach '+JSON.stringify(action.until));
        await page.waitForTimeout(action.wait||500);
      }
      console.log('STEP',step,JSON.stringify(await page.evaluate(()=>Module.meleeState)));
      if(pageErrors.length || await page.evaluate(()=>!!window.probeAbort)) break;
      await page.locator('#canvas').screenshot({path:path.join(root,`browser-step-${step}.png`)});
    }
  }
  if(game && process.env.MELEE_EXPECT_KINDS) {
    const expected=process.env.MELEE_EXPECT_KINDS.split(',').map(Number);
    const state=await page.evaluate(()=>Module.meleeState);
    if(expected.length!==2||state?.mode!==2||state.scene!==2||
       !state.fighters.every((fighter,slot)=>fighter[0]===1&&fighter[1]===expected[slot]))
      throw new Error('Unexpected selected fighters: '+JSON.stringify({expected,state}));
    console.log('PASS requested fighter kinds',JSON.stringify(expected));
  }
  if(game && process.env.MELEE_TEST_LIFECYCLE && !pageErrors.length) {
    for(let cycle=0;cycle<3;cycle++){
      await page.evaluate(()=>Module.setPaused(true));
      await page.waitForFunction(()=>Module.gamePaused===true,undefined,{timeout:10000});
      const before=await page.evaluate(()=>Module.meleeState);
      await page.waitForTimeout(700);
      const paused=await page.evaluate(()=>({state:Module.meleeState,audio:Module.audioContext.state}));
      if(JSON.stringify(paused.state)!==JSON.stringify(before)||paused.audio!=='suspended')
        throw new Error('Simulation or audio continued during pause');
      await page.evaluate(()=>Module.setPaused(false));
      await page.waitForFunction(frame=>!Module.gamePaused && Module.meleeState.frame>frame+5,before.frame,{timeout:10000});
      if(await page.evaluate(()=>Module.audioContext.state)!=='running')throw new Error('Audio did not resume');
      if(pageErrors.length)throw new Error(pageErrors.join('\n'));
    }
    console.log('PASS three browser pause/resume cycles: frozen game snapshots and suspended/resumed audio');
  }
  if(game && process.env.MELEE_TEST_MOVES && !pageErrors.length) {
    for(const [slot,attack,jump] of [[0,'KeyJ','KeyU'],[1,'Numpad1','Numpad4']]) {
      for(const [name,key,accept] of [
        ['attack',attack,f=>f[4]>=44 && f[4]<=69],
        ['jump',jump,f=>f[4]>=24 && f[4]<=28],
      ]) {
        // Entry animations can outlast stage loading, especially on Venom.
        // Send a fresh press only once this fighter is standing and controllable.
        await page.waitForFunction(slot=>window.probeAbort ||
          (Module.meleeState?.fighters?.[slot]?.[0]===1 &&
           Module.meleeState.fighters[slot][4]===14),slot,{timeout:30000});
        if(pageErrors.length)throw new Error(pageErrors.join('\n'));
        let observed=false;const motions=[],samples=[];
        await page.keyboard.down(key);
        try {
          const actionDeadline=performance.now()+10000;
          const startFrame=await page.evaluate(()=>Module.meleeState.frame);
          while(performance.now()<actionDeadline) {
            await page.waitForTimeout(50);
            const state=await page.evaluate(()=>Module.meleeState);
            samples.push({frame:state?.frame,fighter:state?.fighters?.[slot],keys:await page.evaluate(()=>Array.from(Module.meleeKeys||[]))});
            if(pageErrors.length)throw new Error(pageErrors.join('\n'));
            const fighter=state?.fighters?.[slot];
            if(fighter?.[0]){motions.push(fighter[4]);observed ||= accept(fighter);}
            if((observed&&samples.length>=12)||state?.frame>=startFrame+36)break;
          }
        } finally { await page.keyboard.up(key); }
        if(!observed){
          const stacks=await captureWorkerStacks(browser).catch(error=>({error:String(error)}));
          await fs.writeFile(path.join(root,'browser-action-failure.json'),JSON.stringify({slot,name,samples,stacks},null,2));
          await page.locator('#canvas').screenshot({path:path.join(root,'browser-action-failure.png')});
          throw new Error('Player '+(slot+1)+' '+name+' motion was not observed: '+JSON.stringify(samples));
        }
        console.log('PASS player',slot+1,name,'motions',JSON.stringify(motions));
        await page.waitForTimeout(2000);
      }
    }
  }
  if(game && process.env.MELEE_TEST_CONTROLS && !pageErrors.length) {
    const snapshot=()=>page.evaluate(()=>Module.meleeState);
    for(const [slot,key] of [[0,'KeyD'],[1,'ArrowLeft']]) {
      const before=await snapshot();
      if(before?.mode!==2 || before.scene!==2 || !before.fighters.every(f=>f[0]===1))
        throw new Error('Control test requires two live versus fighters');
      await page.keyboard.down(key);
      try { await page.waitForTimeout(300); } finally { await page.keyboard.up(key); }
      await page.waitForTimeout(150);
      const after=await snapshot();
      if(pageErrors.length)throw new Error(pageErrors.join('\n'));
      if(after.frame<=before.frame || Math.abs(after.fighters[slot][2]-before.fighters[slot][2])<1)
        throw new Error('Player '+(slot+1)+' did not move: '+JSON.stringify({before,after}));
      console.log('PASS keyboard movement player',slot+1,JSON.stringify({before:before.fighters,after:after.fighters}));
    }
  }
  if(game && process.env.MELEE_TEST_GAMEPADS && !pageErrors.length) {
    // Exercise the standard browser mapping through the real polling path.
    // This is synthetic Gamepad API coverage, not a physical-controller test.
    await page.evaluate(()=>{
      window.testPads=[0,1].map(index=>({index,id:'Test standard controller',connected:true,mapping:'standard',
        axes:[0,0,0,0],buttons:Array.from({length:16},()=>({pressed:false,value:0}))}));
      Object.defineProperty(navigator,'getGamepads',{configurable:true,value:()=>window.testPads});
    });
    for(const slot of [0,1]) {
      const before=await page.evaluate(()=>Module.meleeState);
      if(before?.mode!==2 || before.scene!==2 || !before.fighters.every(f=>f[0]===1))
        throw new Error('Gamepad test requires two live versus fighters');
      await page.evaluate(slot=>{testPads[slot].axes[0]=slot?-1:1;},slot);
      await page.waitForTimeout(300);
      await page.evaluate(slot=>{testPads[slot].axes[0]=0;},slot);
      await page.waitForTimeout(150);
      const after=await page.evaluate(()=>Module.meleeState);
      if(pageErrors.length || after.frame<=before.frame || Math.abs(after.fighters[slot][2]-before.fighters[slot][2])<1)
        throw new Error('Gamepad movement failed for player '+(slot+1));
      console.log('PASS standard Gamepad API movement player',slot+1,JSON.stringify({before:before.fighters,after:after.fighters}));
    }
    await page.evaluate(()=>{delete navigator.getGamepads;delete window.testPads;});
  }
  async function waitResults(timeout,exercise=false) {
    const deadline=performance.now()+timeout;
    let lastFrame=-1,lastProgress=performance.now();
    let completed=false;
    while(performance.now()<deadline) {
      const state=await page.evaluate(()=>Module.meleeState);
      const diagnostics=await page.evaluate(()=>({phase:Module.netNativePhase?.(),paused:Module.gamePaused,
        alarm:Module.inputAlarmDiagnostics?.(),hidden:document.hidden,heapBytes:HEAPU8.buffer.byteLength,profile:Module.runtimeProfile}));
      if(state?.frame!==lastFrame){lastFrame=state?.frame;lastProgress=performance.now();}
      if(performance.now()-lastProgress>30000){
        const stacks=await captureWorkerStacks(browser).catch(error=>({error:String(error)}));
        await fs.writeFile(path.join(root,'browser-stall-stacks.json'),JSON.stringify(stacks,null,2));
        console.log('STALL STACKS',JSON.stringify(stacks));
        await page.locator('#canvas').screenshot({path:path.join(root,'browser-stall.png')});
        throw new Error('No game-frame progress for 30 seconds: '+JSON.stringify({state,diagnostics}));
      }
      if(pageErrors.length || await page.evaluate(()=>!!window.probeAbort))
        throw new Error('Game stopped while awaiting match results');
      if(state?.mode===2 && state.scene===4){completed=true;break;}
      if(exercise&&state?.mode===2&&(state.scene===2||state.scene===3)){
        const keys=state.scene===3?['KeyA']:['KeyJ','Numpad1'];
        if(state.scene===2&&state.fighters.every(f=>f[0]===1)){
          const dx=state.fighters[1][2]-state.fighters[0][2];
          if(Math.abs(dx)>15)keys.push(dx>0?'KeyD':'KeyA',dx>0?'ArrowLeft':'ArrowRight');
        }
        for(const key of keys)await page.keyboard.down(key);
        try{await page.waitForTimeout(300);}finally{for(const key of keys)await page.keyboard.up(key);}
      }
      console.log('MATCH RUN',JSON.stringify({state,diagnostics}));
      await page.waitForTimeout(10000);
    }
    if(!completed)throw new Error('Match did not reach results before deadline');
    console.log('PASS browser match reached results');
  }
  async function selectStage(target='onett') {
    if(!['onett','venom','fountain','greatbay'].includes(target))throw new Error('Unknown stage target: '+target);
    if(!await page.evaluate(()=>Module.meleeState?.mode===2&&Module.meleeState.scene===1))
      throw new Error('Cursor guidance requires stage selection');
    await page.evaluate(()=>{
      window.rematchPad={index:0,id:'Test rematch controller',connected:true,mapping:'standard',
        axes:[0,0,0,0],buttons:Array.from({length:16},()=>({pressed:false,value:0}))};
      window.rematchPadDescriptor=Object.getOwnPropertyDescriptor(navigator,'getGamepads');
      Object.defineProperty(navigator,'getGamepads',{configurable:true,value:()=>[rematchPad]});
    });
    const selectDeadline=performance.now()+30000;
    try {
      while(performance.now()<selectDeadline){
        const state=await page.evaluate(()=>Module.meleeState);
        if(state?.mode===2&&state.scene!==1)break;
        const guidance=state?.stageTargets?.[target]||0;
        if(!(guidance&16))throw new Error('No cursor guidance for '+target);
        // A full keyboard axis can jump across the 1.2-unit target region.
        // Small real Gamepad API pulses converge without editing game state.
        await page.evaluate(g=>{
          rematchPad.axes[0]=(g&1)?-0.45:(g&2)?0.45:0;
          rematchPad.axes[1]=(g&4)?0.45:(g&8)?-0.45:0;
          rematchPad.buttons[0]={pressed:g===16,value:g===16?1:0};
        },guidance);
        await page.waitForTimeout(guidance===16?100:40);
        await page.evaluate(()=>{rematchPad.axes.fill(0);rematchPad.buttons[0]={pressed:false,value:0};});
        await page.waitForTimeout(guidance===16?500:120);
      }
    } finally {
      await page.evaluate(()=>{
        if(rematchPadDescriptor)Object.defineProperty(navigator,'getGamepads',rematchPadDescriptor);
        else delete navigator.getGamepads;
        delete window.rematchPad;delete window.rematchPadDescriptor;
      });
    }
    await page.waitForFunction(()=>window.probeAbort || (Module.meleeState?.mode===2&&
      Module.meleeState.scene===2&&Module.meleeState.fighters.every(f=>f[0]===1)),undefined,{timeout:60000});
    await page.waitForTimeout(1000);
    const state=await page.evaluate(()=>Module.meleeState);
    if(state?.mode!==2 || state.scene!==2 || !state.fighters.every(f=>f[0]===1))
      throw new Error('Stage selection did not create two live fighters: '+JSON.stringify(state));
  }
  async function rematch() {
    const press=async(keys,hold=100,wait=1800)=>{
      for(const key of keys)await page.keyboard.down(key);
      await page.waitForTimeout(hold);
      for(const key of keys)await page.keyboard.up(key);
      await page.waitForTimeout(wait);
      console.log('REMATCH',JSON.stringify(await page.evaluate(()=>Module.meleeState)));
      if(pageErrors.length)throw new Error(pageErrors.join('\n'));
    };
    await page.waitForTimeout(5000);
    for(let attempt=0;attempt<10;attempt++) {
      if(await page.evaluate(()=>Module.meleeState?.scene===0))break;
      await press(['Enter','NumpadEnter']);
    }
    if(!await page.evaluate(()=>Module.meleeState?.mode===2 && Module.meleeState.scene===0))
      throw new Error('Results did not return to character select');
    await press(['Enter'],100,3500);
    if(await page.evaluate(()=>Module.meleeState?.scene===0)){
      await press(['KeyJ','Numpad1']);await press(['Enter'],100,3500);
    }
    if(!await page.evaluate(()=>Module.meleeState?.scene===1))throw new Error('Rematch did not reach stage selection');
    await selectStage();
    console.log('PASS browser returned from results and started a second match');
  }
  if(game && process.env.MELEE_WAIT_RESULTS && !pageErrors.length)
    await waitResults(Number(process.env.MELEE_WAIT_RESULTS));
  if(game && process.env.MELEE_TEST_REMATCH && !pageErrors.length) {
    if(!process.env.MELEE_WAIT_RESULTS)throw new Error('Rematch requires a completed-match run');
    await rematch();
  }
  if(game && process.env.MELEE_SOAK_MS) {
    const duration=Number(process.env.MELEE_SOAK_MS);
    if(!Number.isFinite(duration)||duration<1||duration>3600000)throw new Error('Invalid soak duration');
    const started=Date.now(),samples=[];
    const measure=()=>page.evaluate(()=>({frame:Module.meleeState.frame,heapBytes:HEAPU8.buffer.byteLength,
      jsHeapBytes:performance.memory?.usedJSHeapSize,audio:Module.audioStats}));
    samples.push(await measure());
    let completed=0;
    while(Date.now()-started<duration) {
      await waitResults(360000,true);completed++;
      await rematch();
      samples.push(await measure());
      await page.locator('#canvas').screenshot({path:path.join(root,'browser-soak-'+completed+'.png')});
      console.log('SOAK',JSON.stringify({elapsedMs:Date.now()-started,completed,sample:samples.at(-1)}));
    }
    const report={elapsedMs:Date.now()-started,completed,samples};
    await fs.writeFile(path.join(root,'browser-soak-result.json'),JSON.stringify(report,null,2));
    if(pageErrors.length||samples.at(-1).frame<=samples[0].frame)throw new Error('Soak stopped advancing');
    if(samples.length>=3&&samples.at(-1).heapBytes-samples[1].heapBytes>128*1024*1024)
      throw new Error('Wasm heap grew by more than 128 MiB after the first rematch');
    console.log('PASS browser gameplay soak',JSON.stringify({elapsedMs:report.elapsedMs,completed}));
  }
  if(game) await page.locator("#canvas").screenshot({path:path.join(root,"browser-boot.png")});
  const result = await page.evaluate(()=>({exit:probeExit,abort:window.probeAbort,messages:probeMessages,audio:Module.audioStats,state:Module.meleeState,profile:Module.runtimeProfile,hitches:Module.runtimeHitches||[]}));
  result.pageErrors=pageErrors;
  await fs.writeFile(path.join(root,`${probe}-result.json`),JSON.stringify(result,null,2));
  if(pageErrors.length || result.abort || !(game ? result.messages.some(x=>x.includes('Browser game frame 120')) :
    result.exit===0 && result.messages.some(x=>x.startsWith('PASS browser WebGPU')))) {
    throw new Error(JSON.stringify(result));
  }
  if(process.env.MELEE_EXPECT_MATCH && !process.env.MELEE_WAIT_RESULTS && !(result.state?.mode===2 && result.state?.scene===2 && result.state.fighters.every(f=>f[0]===1)))
    throw new Error('Expected two live fighters in a versus match: '+JSON.stringify(result.state));
  console.log(game?'PASS requested browser game checks':'PASS actual browser GPU bridge');
} finally { await browser.close(); }
