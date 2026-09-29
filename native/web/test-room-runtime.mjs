/** Actual workerd/SQLite Durable Object integration. Temporary local runtime
 * listeners are needed by Miniflare and are disposed in finally. No deployment. */
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const {Miniflare,convertV4MiniflareOptions}=await import(process.env.MINIFLARE_MODULE||'../build/worker-tools/node_modules/miniflare/dist/src/index.js');
let issued=0;
const runtime=new Miniflare(convertV4MiniflareOptions({host:'127.0.0.1',port:0,workers:[{
 name:'melee-test',modules:true,scriptPath:fileURLToPath(new URL('./server/worker.mjs',import.meta.url)),
 compatibilityDate:'2026-09-01',
 durableObjects:{ROOMS:{className:'MeleeRoom',useSQLite:true}},
 ratelimits:{ROOM_LIMITER:{namespace_id:'1001',simple:{limit:6,period:60}}},
 assets:{directory:fileURLToPath(new URL('../build/wasm-site/',import.meta.url)),binding:'ASSETS',run_worker_first:true},
 bindings:{TURN_KEY_ID:'test-key',TURN_KEY_API_TOKEN:'test-server-only-secret'},
 outboundService:async request=>{
  assert.equal(request.url,'https://rtc.live.cloudflare.com/v1/turn/keys/test-key/credentials/generate-ice-servers');
  assert.equal(request.headers.get('Authorization'),'Bearer test-server-only-secret');
  assert.equal((await request.json()).ttl,3600);issued++;
  return Response.json({iceServers:[{urls:['turn:test.invalid'],username:'temporary',credential:'temporary'}]});
 },
}]}));
async function until(predicate){
 const deadline=Date.now()+5000;
 while(!(await predicate())){if(Date.now()>deadline)throw new Error('Runtime condition timed out');await new Promise(r=>setTimeout(r,20));}
}
const sockets=[];
try{
 const worker=await runtime.getWorker('melee-test');
 const call=(url,init)=>worker.fetch('https://melee.test'+url,init);
 const asset=await call('/');const assetBody=await asset.text();assert.equal(asset.status,200,assetBody);assert.ok(assetBody.includes('Melee Browser'));
 assert.equal(asset.headers.get('Cross-Origin-Opener-Policy'),'same-origin');assert.equal(asset.headers.get('Cross-Origin-Embedder-Policy'),'require-corp');
 const created=await call('/api/rooms',{method:'POST',body:JSON.stringify({delay:4})});assert.equal(created.status,201);
 const room=await created.json(),base='/api/rooms/'+room.id;
 const connect=token=>call(base+'/connect',{headers:{Upgrade:'websocket','Sec-WebSocket-Protocol':'melee-room-v1, '+token}});
 for(const token of [room.hostToken,room.joinToken]){
  const response=await connect(token);assert.equal(response.status,101);
  const socket=response.webSocket,messages=[];sockets.push(socket);
  socket.addEventListener('message',event=>messages.push(JSON.parse(event.data)));socket.accept();socket.messages=messages;
 }
 await until(()=>sockets.every(s=>s.messages.some(m=>m.type==='peer-ready')));
 assert.deepEqual(sockets.map(s=>s.messages.find(m=>m.type==='welcome').slot),[0,1]);
 assert.deepEqual(sockets.map(s=>s.messages.find(m=>m.type==='welcome').delay),[4,4]);
 assert.equal((await connect(room.hostToken)).status,409);
 const ice=()=>call(base+'/ice',{headers:{Authorization:'Bearer '+room.hostToken}});
 const credentials=await Promise.all([ice(),ice(),ice()]);assert.ok(credentials.every(r=>r.status===200));assert.equal(issued,1);
 assert.ok(!(await credentials[0].text()).includes('test-server-only-secret'));
 sockets[0].send(JSON.stringify({description:{type:'offer',sdp:'test offer'}}));
 await until(()=>sockets[1].messages.some(m=>m.type==='signal'));
 assert.equal(sockets[1].messages.find(m=>m.type==='signal').signal.description.sdp,'test offer');
 sockets[0].close(1000,'Test finished');
 await until(()=>sockets[1].messages.some(m=>m.type==='closed'));
 await until(async()=>(await ice()).status===404);
 console.log('PASS actual workerd: assets/isolation, SQLite rooms, WebSocket ownership/signaling, TURN cache and disconnect cleanup');
}finally{
 for(const socket of sockets){try{socket.close();}catch{}}
 await runtime.dispose();
}
