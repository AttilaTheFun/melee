import {PeerTransport} from './transport.mjs';
import {GameSession} from './session.mjs';
async function requestJSON(url,options){
 const response=await fetch(url,{...options,cache:'no-store',signal:AbortSignal.timeout(15000)}),body=await response.json();
 if(!response.ok)throw new Error(body.error||'Room request failed');return body;
}
export const createRoom=(delay=3)=>requestJSON('/api/rooms',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({delay})});
export function parseInvitation(value){
 let code=value.trim();try{code=new URL(code).hash.slice(1);}catch{}
 const match=/^([a-f0-9]{64})\.([a-f0-9]{64})$/.exec(code);
 if(!match)throw new Error('Paste a complete Melee invitation link or code');
 return {id:match[1],token:match[2]};
}
export function invitationURL(room){
 const url=new URL(location.href);url.hash=room.id+'.'+room.joinToken;return url.href;
}
export class RoomConnection {
 constructor({id,token,onClose=()=>{}}){
  if(!/^[a-f0-9]{64}$/.test(id)||!/^[a-f0-9]{64}$/.test(token))throw new Error('Invalid room identity');
  this.id=id;this.token=token;this.onClose=onClose;this.pending=[];this.closed=false;
  this.welcome=new Promise((resolve,reject)=>{this.resolveWelcome=resolve;this.rejectWelcome=reject;});
  this.ready=new Promise((resolve,reject)=>{this.resolveReady=resolve;this.rejectReady=reject;});
  this.welcome.catch(()=>{});this.ready.catch(()=>{});
  const url=new URL('/api/rooms/'+id+'/connect',location.href);url.protocol=url.protocol==='https:'?'wss:':'ws:';
  this.socket=new WebSocket(url,['melee-room-v1',token]);
  this.timer=setTimeout(()=>this.close('Could not join room'),15000);
  this.socket.onclose=()=>this.close('Room connection closed');
  this.socket.onerror=()=>this.close('Could not connect to room');
  this.socket.onmessage=event=>{
   try{
    if(typeof event.data!=='string'||event.data.length>65536)throw new Error('Invalid room response');
    const message=JSON.parse(event.data);
    if(message.type==='welcome'){
     if(this.details||![0,1].includes(message.slot)||!Number.isInteger(message.seed)||message.seed<0||message.seed>0xffffffff||!Number.isInteger(message.delay)||message.delay<0||message.delay>12)
       throw new Error('Invalid room welcome');
     clearTimeout(this.timer);this.details=message;this.resolveWelcome(message);
    }else if(message.type==='peer-ready'){if(!this.details)throw new Error('Room ready before welcome');this.resolveReady();}
    else if(message.type==='closed')this.close(message.reason||'Room closed');
    else if(message.type==='signal'){
     if(this.handler)Promise.resolve(this.handler(message.signal)).catch(()=>this.close('Signaling failed'));
     else{if(this.pending.length>=128)throw new Error('Too much queued signaling');this.pending.push(message.signal);}
    }else throw new Error('Unknown room response');
   }catch(error){this.close(error.message);}
  };
 }
 attach(handler){
  this.handler=handler;
  for(const signal of this.pending)Promise.resolve(handler(signal)).catch(()=>this.close('Signaling failed'));
  this.pending=[];
 }
 send(signal){
  if(this.closed||this.socket.readyState!==WebSocket.OPEN)throw new Error('Room disconnected');
  this.socket.send(JSON.stringify(signal));
 }
 async ice(){return requestJSON('/api/rooms/'+this.id+'/ice',{headers:{Authorization:'Bearer '+this.token}});}
 close(reason='Disconnected'){
  if(this.closed)return;this.closed=true;clearTimeout(this.timer);this.pending=[];
  this.rejectWelcome(new Error(reason));this.rejectReady(new Error(reason));this.socket.close();this.onClose(reason);
 }
}
export async function connectGameSession(room,{build,disc,relayOnly=false,onStatus=()=>{}}){
 const details=await room.welcome;
 onStatus('Waiting for the other player…');await room.ready;
 onStatus('Connecting players…');
 const {iceServers}=await room.ice();
 if(room.closed)throw new Error('Room disconnected');
 const transport=new PeerTransport({initiator:details.slot===0,iceServers,relayOnly,
  onSignal:signal=>room.send(signal),onPacket:()=>{},onClose:reason=>room.close(reason)});
 const session=new GameSession({transport,localSlot:details.slot,seed:details.seed,delay:details.delay,build,disc});
 const previousClose=room.onClose;room.onClose=reason=>{session.close(reason);previousClose(reason);};
 room.attach(signal=>transport.acceptSignal(signal));
 try{
  if(details.slot===0)await transport.offer();
  await session.start();
  const route=await transport.route();
  onStatus('Connected as player '+(details.slot+1)+(route?.relay?' through TURN':'')+' · '+details.delay+' ticks input delay.');
  return session;
 }catch(error){session.close(error.message);throw error;}
}
