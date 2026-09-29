/** Real headless WebRTC, isolated browser contexts, no HTTP listening server. */
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE||'playwright');
const source=await fs.readFile(new URL('./netplay/transport.mjs',import.meta.url),'utf8');
const iceServers=process.env.MELEE_ICE_CONFIG?JSON.parse(await fs.readFile(process.env.MELEE_ICE_CONFIG,'utf8')):[];
const relayOnly=!!process.env.MELEE_RELAY_ONLY;
const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_BINARY||'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'});
try {
  const contexts=await Promise.all([browser.newContext(),browser.newContext()]);
  for(const context of contexts)await context.route('https://melee.test/**',route=>route.fulfill({
    contentType:route.request().url().endsWith('.mjs')?'text/javascript':'text/html',
    body:route.request().url().endsWith('.mjs')?source:'<!doctype html><title>Melee transport test</title>',
  }));
  const pages=await Promise.all(contexts.map(c=>c.newPage()));
  const errors=[];for(const page of pages)page.on('pageerror',e=>errors.push(String(e)));
  for(let slot=0;slot<2;slot++){
    await pages[slot].exposeFunction('signal',message=>pages[1-slot].evaluate(message=>window.peer.acceptSignal(message),message));
    await pages[slot].goto('https://melee.test/');
    await pages[slot].evaluate(async({slot,iceServers,relayOnly})=>{
      const {PeerTransport}=await import('/transport.mjs');
      window.received=[];window.closedReason=null;
      window.peer=new PeerTransport({initiator:slot===0,iceServers,relayOnly,onSignal:signal,
        onPacket:packet=>{received.push(Array.from(packet));},onClose:reason=>{closedReason=reason;}});
    },{slot,iceServers,relayOnly});
  }
  await pages[0].evaluate(()=>peer.offer());
  await Promise.all(pages.map(p=>p.evaluate(()=>peer.ready)));
  await Promise.all(pages.map((page,slot)=>page.evaluate(slot=>{
    for(let i=0;i<1000;i++){
      const packet=new Uint8Array(32);new DataView(packet.buffer).setUint32(0,i);
      for(let j=4;j<packet.length;j++)packet[j]=(i+j+slot)&255;
      peer.send(packet);
    }
  },slot)));
  for(const page of pages)await page.waitForFunction(()=>received.length===1000,undefined,{timeout:30000});
  for(let slot=0;slot<2;slot++){
    const received=await pages[slot].evaluate(()=>received);
    for(let i=0;i<received.length;i++){
      const packet=Uint8Array.from(received[i]);assert.equal(new DataView(packet.buffer).getUint32(0),i);
      for(let j=4;j<packet.length;j++)assert.equal(packet[j],(i+j+1-slot)&255);
    }
    const route=await pages[slot].evaluate(()=>peer.route());
    assert.ok(route);if(relayOnly)assert.equal(route.relay,true,'Forced TURN test used a non-relay path');
    console.log('PASS peer',slot,'1000 ordered packets; route',JSON.stringify(route));
  }
  await pages[0].evaluate(()=>{
    let rejected=false;try{peer.send(new Uint8Array(65537));}catch{rejected=true;}
    if(!rejected||peer.closed)throw new Error('Local packet size validation failed');
    peer.channel.send('malformed input');
  });
  await pages[1].waitForFunction(()=>peer.closed,undefined,{timeout:10000});
  assert.equal(await pages[1].evaluate(()=>closedReason),'Invalid input packet');
  await pages[0].waitForFunction(()=>peer.closed,undefined,{timeout:10000});
  await pages[0].evaluate(()=>{let rejected=false;try{peer.send(new Uint8Array(1));}catch{rejected=true;}if(!rejected)throw new Error('Send after close accepted');});
  assert.deepEqual(errors,[]);
  console.log('PASS WebRTC transport bidirectional integrity and disconnect cleanup'+(relayOnly?' over forced TURN':' (direct; TURN not verified)'));
}finally{await browser.close();}
