/** Cloudflare Worker + one Durable Object per private two-player room.
 * Publish only the asset staging directory, never native/build or disc files. */
const json=(value,status=200)=>Response.json(value,{status,headers:{'Cache-Control':'no-store'}});
const randomToken=()=>Array.from(crypto.getRandomValues(new Uint8Array(32)),x=>x.toString(16).padStart(2,'0')).join('');
const ROOM=/^\/api\/rooms\/([a-f0-9]{64})\/(connect|ice)$/;
export function validSignal(value){
 if(!value||typeof value!=='object')return false;
 if(value.description)return ['offer','answer'].includes(value.description.type)&&typeof value.description.sdp==='string'&&value.description.sdp.length<=60000;
 if(value.candidate)return typeof value.candidate.candidate==='string'&&value.candidate.candidate.length<=4096;
 return false;
}
async function roomOptions(request){
 if(!request.body)return {delay:3};
 const reader=request.body.getReader(),chunks=[];let size=0;
 try{
  for(;;){const {done,value}=await reader.read();if(done)break;
   size+=value.length;if(size>256){await reader.cancel();throw new Error('Request too large');}chunks.push(value);
  }
 }finally{reader.releaseLock();}
 const bytes=new Uint8Array(size);let offset=0;for(const chunk of chunks){bytes.set(chunk,offset);offset+=chunk.length;}
 const text=new TextDecoder('utf-8',{fatal:true}).decode(bytes).trim();
 const options=text?JSON.parse(text):{};
 if(!options||typeof options!=='object'||Array.isArray(options)||Object.keys(options).some(key=>key!=='delay'))throw new Error('Invalid room options');
 const delay=options.delay===undefined?3:options.delay;
 if(!Number.isInteger(delay)||delay<0||delay>12)throw new Error('Invalid input delay');
 return {delay};
}
export default {
 async fetch(request,env){
  const url=new URL(request.url);
  if(!url.pathname.startsWith('/api/')){
   if(!env.ASSETS)return new Response('Not found',{status:404});
   const response=await env.ASSETS.fetch(request),headers=new Headers(response.headers);
   headers.set('Cross-Origin-Opener-Policy','same-origin');headers.set('Cross-Origin-Embedder-Policy','require-corp');
   return new Response(response.body,{status:response.status,headers});
  }
  const origin=request.headers.get('Origin');
  if(origin&&origin!==url.origin)return json({error:'Origin rejected'},403);
  if(url.pathname==='/api/rooms'&&request.method==='POST'){
   if(!env.ROOM_LIMITER)return json({error:'Room service is not configured'},503);
   const {success}=await env.ROOM_LIMITER.limit({key:request.headers.get('CF-Connecting-IP')||'unknown'});
   if(!success)return json({error:'Too many rooms; try again shortly'},429);
   let options;try{options=await roomOptions(request);}catch{return json({error:'Invalid room options'},400);}
   const id=randomToken(),hostToken=randomToken(),joinToken=randomToken();
   const room={hostToken,joinToken,delay:options.delay,seed:crypto.getRandomValues(new Uint32Array(1))[0],expiresAt:Date.now()+3600000};
   const stub=env.ROOMS.get(env.ROOMS.idFromName(id));
   const initialized=await stub.fetch(new Request('https://room/init',{method:'POST',body:JSON.stringify(room)}));
   if(!initialized.ok)return json({error:'Could not create room'},502);
   return json({id,...room},201);
  }
  const match=ROOM.exec(url.pathname);
  if(match&&request.method==='GET')return env.ROOMS.get(env.ROOMS.idFromName(match[1])).fetch(request);
  return json({error:'Not found'},404);
 }
};
export class MeleeRoom {
 constructor(ctx,env){this.ctx=ctx;this.env=env;this.iceRequests=new Map();}
 async fetch(request){
  const url=new URL(request.url);
  if(url.pathname==='/init'&&request.method==='POST'){
   const room=await request.json();
   if(await this.ctx.storage.get('room'))return json({error:'Already initialized'},409);
   await this.ctx.storage.put('room',room);await this.ctx.storage.setAlarm(room.expiresAt);
   return json({ok:true});
  }
  const room=await this.ctx.storage.get('room');
  if(!room||Date.now()>=room.expiresAt)return json({error:'Room expired or closed'},404);
  const connecting=url.pathname.endsWith('/connect');
  const protocols=(request.headers.get('Sec-WebSocket-Protocol')||'').split(',').map(x=>x.trim());
  const token=connecting?(protocols[0]==='melee-room-v1'?protocols[1]:null):(request.headers.get('Authorization')||'').replace(/^Bearer /,'');
  const slot=token===room.hostToken?0:token===room.joinToken?1:-1;
  if(slot<0)return json({error:'Invalid invitation'},403);
  if(url.pathname.endsWith('/ice')){
   if(!this.env.TURN_KEY_ID||!this.env.TURN_KEY_API_TOKEN)return json({error:'TURN is not configured'},503);
   if(!this.iceRequests.has(slot))this.iceRequests.set(slot,this.issueIce(room,slot).finally(()=>this.iceRequests.delete(slot)));
   try{return json(await this.iceRequests.get(slot));}
   catch{return json({error:'TURN credential service failed'},502);}
  }
  if(!connecting||request.headers.get('Upgrade')?.toLowerCase()!=='websocket')return json({error:'WebSocket required'},426);
  if(this.ctx.getWebSockets().some(ws=>ws.deserializeAttachment()?.slot===slot))return json({error:'Player slot already connected'},409);
  const [client,server]=Object.values(new WebSocketPair());
  this.ctx.acceptWebSocket(server);server.serializeAttachment({slot,count:0});
  server.send(JSON.stringify({type:'welcome',slot,seed:room.seed,delay:room.delay,expiresAt:room.expiresAt}));
  if(this.ctx.getWebSockets().length===2)for(const ws of this.ctx.getWebSockets())ws.send(JSON.stringify({type:'peer-ready'}));
  return new Response(null,{status:101,webSocket:client,headers:{'Sec-WebSocket-Protocol':'melee-room-v1'}});
 }
 async issueIce(room,slot){
  const cached=await this.ctx.storage.get('ice-'+slot);if(cached)return cached;
  const response=await fetch('https://rtc.live.cloudflare.com/v1/turn/keys/'+encodeURIComponent(this.env.TURN_KEY_ID)+'/credentials/generate-ice-servers',{
    method:'POST',headers:{Authorization:'Bearer '+this.env.TURN_KEY_API_TOKEN,'Content-Type':'application/json'},
    body:JSON.stringify({ttl:3600}),signal:AbortSignal.timeout(10000)});
  if(!response.ok)throw new Error('TURN upstream failed');
  const credentials=await response.json();
  if(!Array.isArray(credentials.iceServers))throw new Error('Invalid TURN response');
  const current=await this.ctx.storage.get('room');
  if(!current||current.expiresAt!==room.expiresAt||Date.now()>=current.expiresAt)throw new Error('Room closed');
  await this.ctx.storage.put('ice-'+slot,credentials);return credentials;
 }
 async webSocketMessage(ws,message){
  const attachment=ws.deserializeAttachment();
  if(typeof message!=='string'||message.length>65536||!attachment||++attachment.count>128){await this.end('Invalid signaling traffic');return;}
  ws.serializeAttachment(attachment);
  let value;try{value=JSON.parse(message);}catch{await this.end('Invalid signaling message');return;}
  if(!validSignal(value)){await this.end('Invalid signaling message');return;}
  const other=this.ctx.getWebSockets().find(peer=>peer!==ws);
  if(!other){await this.end('Other player is not connected');return;}
  other.send(JSON.stringify({type:'signal',signal:value}));
 }
 async end(reason){
  for(const ws of this.ctx.getWebSockets()){
   try{ws.send(JSON.stringify({type:'closed',reason}));ws.close(1000,reason);}catch{}
  }
  await this.ctx.storage.deleteAll();
 }
 async webSocketClose(){await this.end('Other player disconnected');}
 async webSocketError(){await this.end('Signaling connection failed');}
 async alarm(){await this.end('Room expired');}
}
