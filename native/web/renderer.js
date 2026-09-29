const shader=`
struct Camera { mvp:mat4x4f };
struct Material { mode:vec4f, alpha:vec4f, test:vec4f };
@group(0) @binding(0) var<uniform> camera:Camera;
@group(1) @binding(0) var tex:texture_2d<f32>;
@group(1) @binding(1) var samp:sampler;
@group(1) @binding(2) var<uniform> mat:Material;
struct Out { @builtin(position) position:vec4f, @location(0) normal:vec3f, @location(1) color:vec4f, @location(2) uv:vec2f };
@vertex fn vs(@location(0) p:vec3f,@location(1) n:vec3f,@location(2) c:vec4f,@location(3) uv:vec2f)->Out {
 var o:Out;o.position=camera.mvp*vec4f(p,1);o.normal=n;o.color=c;o.uv=uv;return o;
}
fn compare(a:f32,b:f32,op:u32)->bool {
 switch op {case 0u:{return false;}case 1u:{return a<b;}case 2u:{return a==b;}case 3u:{return a<=b;}case 4u:{return a>b;}case 5u:{return a!=b;}case 6u:{return a>=b;}default:{return true;}}
}
@fragment fn fs(i:Out)->@location(0) vec4f {
 let t=textureSample(tex,samp,i.uv);
 var c=i.color.rgb*(0.25+0.75*max(dot(normalize(i.normal),normalize(vec3f(0.4,0.8,0.6))),0.0));
 var a=i.color.a;
 switch u32(mat.mode.x) {
 case 1u:{c=mix(c,t.rgb,t.a);}case 2u:{c=mix(c,t.rgb,t.rgb);}case 3u:{c=mix(c,t.rgb,mat.mode.z);}
 case 4u:{c*=t.rgb;}case 5u:{c=t.rgb;}case 7u:{c+=t.rgb;}case 8u:{c-=t.rgb;}default:{}
 }
 switch u32(mat.mode.y) {
 case 1u:{a=mix(a,t.a,t.a);}case 2u:{a=mix(a,t.a,mat.mode.z);}case 3u:{a*=t.a;}case 4u:{a=t.a;}case 6u:{a+=t.a;}case 7u:{a-=t.a;}default:{}
 }
 a=clamp(a,0,1);
 let x=compare(a,mat.alpha.y, u32(mat.alpha.x));let y=compare(a,mat.test.x,u32(mat.alpha.w));
 var accepted=false;
 switch u32(mat.alpha.z) {case 0u:{accepted=x&&y;}case 1u:{accepted=x||y;}case 2u:{accepted=x!=y;}default:{accepted=x==y;}}
 if(!accepted){discard;}return vec4f(c,a);
}`;
const comparisons=['never','less','equal','less-equal','greater','not-equal','greater-equal','always'];
const src=['zero','one','dst','one-minus-dst','src-alpha','one-minus-src-alpha','dst-alpha','one-minus-dst-alpha'];
const dst=['zero','one','src','one-minus-src','src-alpha','one-minus-src-alpha','dst-alpha','one-minus-dst-alpha'];
const srcA=['zero','one','dst-alpha','one-minus-dst-alpha','src-alpha','one-minus-src-alpha','dst-alpha','one-minus-dst-alpha'];
const dstA=['zero','one','src-alpha','one-minus-src-alpha','src-alpha','one-minus-src-alpha','dst-alpha','one-minus-dst-alpha'];
const sub=(a,b)=>a.map((v,i)=>v-b[i]);
const dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0);
const cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];
const norm=a=>a.map(v=>v/Math.hypot(...a));
function camera(aspect,yaw,pitch,distance) {
 const target=[0,8,0],eye=[distance*Math.cos(pitch)*Math.sin(yaw),8+distance*Math.sin(pitch),distance*Math.cos(pitch)*Math.cos(yaw)];
 const z=norm(sub(eye,target)),x=norm(cross([0,1,0],z)),y=cross(z,x);
 const view=[x[0],y[0],z[0],0,x[1],y[1],z[1],0,x[2],y[2],z[2],0,-dot(x,eye),-dot(y,eye),-dot(z,eye),1];
 const f=1/Math.tan(Math.PI/8),near=.1,far=250;
 const p=[f/aspect,0,0,0,0,f,0,0,0,0,far/(near-far),-1,0,0,far*near/(near-far),0];
 const out=new Float32Array(16);
 for(let c=0;c<4;c++)for(let r=0;r<4;r++)for(let k=0;k<4;k++)out[c*4+r]+=p[k*4+r]*view[c*4+k];
 return out;
}
export class Renderer {
 static async create(canvas,onError) {
  if(!navigator.gpu)throw Error('WebGPU is unavailable. Open this page in desktop Safari 26 or newer over localhost or HTTPS.');
  const adapter=await navigator.gpu.requestAdapter();if(!adapter)throw Error('No WebGPU adapter is available.');
  const device=await adapter.requestDevice();
  const r=new Renderer(canvas,device,onError);
  const info=await r.module.getCompilationInfo();
  const errors=info.messages.filter(m=>m.type==='error');if(errors.length)throw Error(errors.map(e=>e.message).join('\n'));
  device.lost.then(info=>onError(Error('WebGPU device lost: '+info.message+' Reload to recover.')));
  device.addEventListener('uncapturederror',e=>onError(e.error));return r;
 }
 constructor(canvas,device,onError) {
  this.canvas=canvas;this.device=device;this.onError=onError;this.parts=[];this.cache=new Map();this.reset();
  this.context=canvas.getContext('webgpu');this.format=navigator.gpu.getPreferredCanvasFormat();
  this.context.configure({device,format:this.format,alphaMode:'opaque'});
  this.module=device.createShaderModule({code:shader});
  this.cameraLayout=device.createBindGroupLayout({entries:[{binding:0,visibility:GPUShaderStage.VERTEX,buffer:{type:'uniform'}}]});
  this.materialLayout=device.createBindGroupLayout({entries:[{binding:0,visibility:GPUShaderStage.FRAGMENT,texture:{}},{binding:1,visibility:GPUShaderStage.FRAGMENT,sampler:{}},{binding:2,visibility:GPUShaderStage.FRAGMENT,buffer:{type:'uniform'}}]});
  this.layout=device.createPipelineLayout({bindGroupLayouts:[this.cameraLayout,this.materialLayout]});
  this.uniform=this.buffer(new Float32Array(16),GPUBufferUsage.UNIFORM);
  this.cameraGroup=device.createBindGroup({layout:this.cameraLayout,entries:[{binding:0,resource:{buffer:this.uniform}}]});
 }
 reset(){this.yaw=.72;this.pitch=.22;this.distance=43;}
 buffer(data,usage) {const b=this.device.createBuffer({size:Math.max(4,data.byteLength),usage:usage|GPUBufferUsage.COPY_DST});if(data.byteLength)this.device.queue.writeBuffer(b,0,data);return b;}
 async pipeline(s) {
  if(s[4]===2||s[2]||s[17])throw Error('This material needs GX logic operations, destination alpha, or dithering outside the prototype renderer.');
  const key=JSON.stringify(s.slice(0,12));if(this.cache.has(key))return this.cache.get(key);
  let blend;
  if(s[4]===1)blend={color:{srcFactor:src[s[5]],dstFactor:dst[s[6]],operation:'add'},alpha:{srcFactor:srcA[s[5]],dstFactor:dstA[s[6]],operation:'add'}};
  if(s[4]===3)blend={color:{srcFactor:'one',dstFactor:'one',operation:'reverse-subtract'},alpha:{srcFactor:'one',dstFactor:'one',operation:'reverse-subtract'}};
  const result=await this.device.createRenderPipelineAsync({layout:this.layout,
   vertex:{module:this.module,entryPoint:'vs',buffers:[{arrayStride:48,attributes:[{shaderLocation:0,offset:0,format:'float32x3'},{shaderLocation:1,offset:12,format:'float32x3'},{shaderLocation:2,offset:24,format:'float32x4'},{shaderLocation:3,offset:40,format:'float32x2'}]}]},
   fragment:{module:this.module,entryPoint:'fs',targets:[{format:this.format,blend,writeMask:(s[0]?7:0)|(s[1]?8:0)}]},
   primitive:{topology:'triangle-list',cullMode:'none'},depthStencil:{format:'depth24plus',depthWriteEnabled:!!(s[8]&&s[10]),depthCompare:s[8]?comparisons[s[9]]:'always'}});
  this.cache.set(key,result);return result;
 }
 clear(){for(const p of this.parts){p.vertex.destroy();p.index.destroy();p.texture.destroy();p.material.destroy();}this.parts=[];}
 async load(parts) {
  this.clear();
  try {for(const p of parts) {
   const pipeline=await this.pipeline(p.pixel),d=this.device;
   const texture=d.createTexture({size:[p.width,p.height],format:'rgba8unorm',usage:GPUTextureUsage.TEXTURE_BINDING|GPUTextureUsage.COPY_DST});
   d.queue.writeTexture({texture},p.pixels,{bytesPerRow:p.width*4},[p.width,p.height]);
   const wraps=['clamp-to-edge','repeat','mirror-repeat'];
   const sampler=d.createSampler({addressModeU:wraps[p.wrap[0]],addressModeV:wraps[p.wrap[1]],magFilter:'linear',minFilter:'linear'});
   const s=p.pixel, material=this.buffer(new Float32Array([...p.settings,s[12],s[13]/255,s[14],s[15],s[16]/255,0,0,0]),GPUBufferUsage.UNIFORM);
   const group=d.createBindGroup({layout:this.materialLayout,entries:[{binding:0,resource:texture.createView()},{binding:1,resource:sampler},{binding:2,resource:{buffer:material}}]});
   this.parts.push({pipeline,texture,material,group,hidden:p.hidden,count:p.indices.length,vertex:this.buffer(p.vertices,GPUBufferUsage.VERTEX),index:this.buffer(p.indices,GPUBufferUsage.INDEX)});
  }}catch(e){this.clear();throw e;}
 }
 update(parts){for(let i=0;i<parts.length;i++){this.device.queue.writeBuffer(this.parts[i].vertex,0,parts[i].vertices);this.parts[i].hidden=parts[i].hidden;}}
 draw() {
  const d=this.device,c=this.canvas,scale=Math.min(devicePixelRatio||1,2);
  const w=Math.min(d.limits.maxTextureDimension2D,Math.max(1,Math.round(c.clientWidth*scale))),h=Math.min(d.limits.maxTextureDimension2D,Math.max(1,Math.round(c.clientHeight*scale)));
  if(!this.depth||c.width!==w||c.height!==h){c.width=w;c.height=h;this.depth?.destroy();this.depth=d.createTexture({size:[w,h],format:'depth24plus',usage:GPUTextureUsage.RENDER_ATTACHMENT});}
  d.queue.writeBuffer(this.uniform,0,camera(w/h,this.yaw,this.pitch,this.distance));
  const encoder=d.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[{view:this.context.getCurrentTexture().createView(),clearValue:{r:.025,g:.035,b:.06,a:1},loadOp:'clear',storeOp:'store'}],depthStencilAttachment:{view:this.depth.createView(),depthClearValue:1,depthLoadOp:'clear',depthStoreOp:'store'}});
  pass.setBindGroup(0,this.cameraGroup);
  for(const p of this.parts)if(!p.hidden&&p.count){pass.setPipeline(p.pipeline);pass.setBindGroup(1,p.group);pass.setVertexBuffer(0,p.vertex);pass.setIndexBuffer(p.index,'uint32');pass.drawIndexed(p.count);}
  pass.end();d.queue.submit([encoder.finish()]);
 }
}
