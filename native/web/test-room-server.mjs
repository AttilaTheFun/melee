/** Worker routing/room tests with in-memory Cloudflare boundary doubles.
 * Deployment/runtime integration is a separate verification gate. */
import assert from 'node:assert/strict';
import worker,{MeleeRoom,validSignal} from './server/worker.mjs';
const NativeResponse=Response,nativeFetch=fetch;
class Socket {constructor(){this.messages=[];this.closed=false;}send(message){this.messages.push(JSON.parse(message));}close(){this.closed=true;}serializeAttachment(a){this.attachment=structuredClone(a);}deserializeAttachment(){return structuredClone(this.attachment);}}
globalThis.WebSocketPair=class{constructor(){this[0]=new Socket();this[1]=new Socket();}};
globalThis.Response=function(body,options){return options?.status===101?{status:101,webSocket:options.webSocket,headers:new Headers(options.headers)}:new NativeResponse(body,options);};
Response.json=NativeResponse.json.bind(NativeResponse);
const data=new Map(),sockets=[];
const ctx={storage:{get:async key=>data.get(key),put:async(key,value)=>{data.set(key,structuredClone(value));},setAlarm:async()=>{},deleteAll:async()=>data.clear()},
 getWebSockets:()=>sockets.filter(s=>!s.closed),acceptWebSocket:ws=>sockets.push(ws)};
const room=new MeleeRoom(ctx,{TURN_KEY_ID:'test-key',TURN_KEY_API_TOKEN:'server-only-secret'});
const env={ROOM_LIMITER:{limit:async()=>({success:true})},ROOMS:{idFromName:id=>id,get:()=>room},ASSETS:{fetch:async()=>new NativeResponse('asset')}};
try{
 const badOrigin=await worker.fetch(new Request('https://melee.test/api/rooms',{method:'POST',headers:{Origin:'https://other.test'}}),env);assert.equal(badOrigin.status,403);
 const created=await worker.fetch(new Request('https://melee.test/api/rooms',{method:'POST'}),env);assert.equal(created.status,201);
 const details=await created.json();assert.match(details.id,/^[a-f0-9]{64}$/);assert.notEqual(details.hostToken,details.joinToken);assert.equal(details.delay,3);
 for(const body of ['{"delay":null}','{"delay":-1}','{"delay":13}','{"delay":2.5}', 'x'.repeat(257)])assert.equal((await worker.fetch(new Request('https://melee.test/api/rooms',{method:'POST',body}),env)).status,400);
 const url='https://melee.test/api/rooms/'+details.id;
 assert.equal((await worker.fetch(new Request(url+'/ice',{headers:{Authorization:'Bearer wrong'}}),env)).status,403);
 const connect=token=>worker.fetch(new Request(url+'/connect',{headers:{Upgrade:'websocket','Sec-WebSocket-Protocol':'melee-room-v1, '+token}}),env);
 assert.equal((await connect(details.hostToken)).status,101);assert.equal((await connect(details.hostToken)).status,409);
 assert.equal((await connect(details.joinToken)).status,101);
 assert.ok(sockets.every(s=>s.messages.some(m=>m.type==='peer-ready')));
 let minted=0;
 globalThis.fetch=async(url,options)=>{
  assert.equal(options.headers.Authorization,'Bearer server-only-secret');assert.equal(JSON.parse(options.body).ttl,3600);minted++;
  await new Promise(resolve=>setTimeout(resolve,5));
  return NativeResponse.json({iceServers:[{urls:['turn:relay.test'],username:'temporary',credential:'temporary-secret'}]},{status:201});
 };
 const ice=()=>worker.fetch(new Request(url+'/ice',{headers:{Authorization:'Bearer '+details.hostToken}}),env);
 const responses=await Promise.all([ice(),ice(),ice()]);assert.ok(responses.every(r=>r.status===200));assert.equal(minted,1);
 assert.ok(!(await responses[0].text()).includes('server-only-secret'));
 await ice();assert.equal(minted,1);
 await room.webSocketMessage(sockets[0],JSON.stringify({description:{type:'offer',sdp:'test-sdp'}}));
 assert.deepEqual(sockets[1].messages.at(-1),{type:'signal',signal:{description:{type:'offer',sdp:'test-sdp'}}});
 assert.equal(validSignal({description:{type:'bogus',sdp:''}}),false);
 await room.webSocketMessage(sockets[0],JSON.stringify({description:{type:'offer',sdp:'x'.repeat(65537)}}));
 assert.ok(sockets.every(s=>s.closed));assert.equal(data.size,0);
 assert.equal((await ice()).status,404);
 const asset=await worker.fetch(new Request('https://melee.test/index.html'),env);
 assert.equal(asset.headers.get('Cross-Origin-Opener-Policy'),'same-origin');assert.equal(asset.headers.get('Cross-Origin-Embedder-Policy'),'require-corp');
 const limited={...env,ROOM_LIMITER:{limit:async()=>({success:false})}};
 assert.equal((await worker.fetch(new Request('https://melee.test/api/rooms',{method:'POST'}),limited)).status,429);
 console.log('PASS room authorization, two-slot ownership, signaling limits, TURN single-flight/cache, secret boundary, cleanup and isolation headers');
}finally{globalThis.Response=NativeResponse;globalThis.fetch=nativeFetch;delete globalThis.WebSocketPair;}
