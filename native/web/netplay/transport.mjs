/** Ordered, reliable input transport. Signaling carries SDP/ICE only; disc data
 * and TURN service secrets never enter this module. */
export class PeerTransport {
  constructor({initiator,iceServers=[],relayOnly=false,onSignal,onPacket,onClose=()=>{},timeoutMs=30000}) {
    if(typeof onSignal!=='function'||typeof onPacket!=='function')throw new TypeError('Missing transport callbacks');
    this.onSignal=onSignal;this.onPacket=onPacket;this.onClose=onClose;
    this.closed=false;this.pendingCandidates=[];this.signalQueue=Promise.resolve();
    this.pc=new RTCPeerConnection({iceServers,iceTransportPolicy:relayOnly?'relay':'all'});
    this.ready=new Promise((resolve,reject)=>{this.resolveReady=resolve;this.rejectReady=reject;});
    // The owner may await ready after exchanging descriptions; avoid an
    // unhandled rejection if connection setup fails first.
    this.ready.catch(()=>{});
    this.timer=setTimeout(()=>this.close('Connection timed out'),timeoutMs);
    this.pc.onicecandidate=event=>{
      if(event.candidate&&!this.closed)Promise.resolve().then(()=>this.onSignal({candidate:event.candidate.toJSON()}))
        .catch(()=>this.close('Signaling failed'));
    };
    this.pc.onconnectionstatechange=()=>{
      const state=this.pc.connectionState;
      if(['failed','closed'].includes(state))this.close('Peer connection '+state);
      else if(state==='disconnected'&&!this.disconnectTimer)this.disconnectTimer=setTimeout(()=>this.close('Peer disconnected'),5000);
      else if(state==='connected'){clearTimeout(this.disconnectTimer);this.disconnectTimer=null;}
    };
    this.pc.ondatachannel=event=>{
      if(initiator||this.channel){event.channel.close();this.close('Unexpected data channel');return;}
      this.attach(event.channel);
    };
    if(initiator)this.attach(this.pc.createDataChannel('melee-input',{ordered:true,protocol:'melee-input-v1'}));
  }
  attach(channel) {
    if(channel.label!=='melee-input'||channel.protocol!=='melee-input-v1'||!channel.ordered||
       channel.maxRetransmits!==null||channel.maxPacketLifeTime!==null){channel.close();this.close('Incompatible input channel');return;}
    this.channel=channel;channel.binaryType='arraybuffer';
    channel.onopen=()=>{clearTimeout(this.timer);this.resolveReady();};
    channel.onclose=()=>this.close('Input channel closed');
    channel.onerror=()=>this.close('Input channel failed');
    channel.onmessage=event=>{
      if(!(event.data instanceof ArrayBuffer)||event.data.byteLength>65536){this.close('Invalid input packet');return;}
      try{this.onPacket(new Uint8Array(event.data));}catch{this.close('Input protocol error');}
    };
  }
  async offer() {
    if(this.closed)throw new Error('Transport closed');
    try {
      await this.pc.setLocalDescription(await this.pc.createOffer());
      await this.onSignal({description:this.pc.localDescription.toJSON()});
    }catch(error){this.close('Offer failed');throw error;}
  }
  acceptSignal(message) {
    this.signalQueue=this.signalQueue.then(async()=>{
      if(this.closed)throw new Error('Transport closed');
      if(message?.description){
        const d=message.description;
        if(!['offer','answer'].includes(d.type)||typeof d.sdp!=='string'||d.sdp.length>65536)throw new Error('Invalid description');
        await this.pc.setRemoteDescription(d);
        for(const candidate of this.pendingCandidates)await this.pc.addIceCandidate(candidate);
        this.pendingCandidates=[];
        if(d.type==='offer'){
          await this.pc.setLocalDescription(await this.pc.createAnswer());
          await this.onSignal({description:this.pc.localDescription.toJSON()});
        }
      }else if(message?.candidate){
        const c=message.candidate;
        if(typeof c.candidate!=='string'||c.candidate.length>4096)throw new Error('Invalid ICE candidate');
        if(this.pc.remoteDescription)await this.pc.addIceCandidate(c);
        else {if(this.pendingCandidates.length>=64)throw new Error('Too many ICE candidates');this.pendingCandidates.push(c);}
      }else throw new Error('Invalid signaling message');
    }).catch(error=>{this.close('Signaling rejected');throw error;});
    return this.signalQueue;
  }
  send(packet) {
    if(this.closed||this.channel?.readyState!=='open')throw new Error('Input transport is not open');
    if(!(packet instanceof Uint8Array)||packet.byteLength>65536)throw new TypeError('Invalid packet');
    if(this.channel.bufferedAmount+packet.byteLength>262144){this.close('Peer is not keeping up');throw new Error('Input queue overflow');}
    this.channel.send(packet);
  }
  async route() {
    const stats=await this.pc.getStats();
    for(const report of stats.values())if(report.type==='transport'&&report.selectedCandidatePairId){
      const pair=stats.get(report.selectedCandidatePairId);
      const local=stats.get(pair?.localCandidateId),remote=stats.get(pair?.remoteCandidateId);
      return {local:local?.candidateType,remote:remote?.candidateType,rtt:pair?.currentRoundTripTime,
        relay:local?.candidateType==='relay'||remote?.candidateType==='relay'};
    }
    return null;
  }
  close(reason='Disconnected') {
    if(this.closed)return;
    this.closed=true;clearTimeout(this.timer);clearTimeout(this.disconnectTimer);this.pendingCandidates=[];
    this.rejectReady(new Error(reason));this.channel?.close();this.pc.close();this.onClose(reason);
  }
}
