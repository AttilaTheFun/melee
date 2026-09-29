// Windowless Metal/Dawn validation. This is NOT a Safari compatibility test.
import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {create,globals} from '../build/web-gpu-test/node_modules/webgpu/index.js';
import createMelee from '../build/web/melee.mjs';
import {Renderer} from './renderer.js';
import {snapshot} from './scene-data.js';
Object.assign(globalThis,globals);
const gpu=create(['backend=metal']);
Object.defineProperty(globalThis,'navigator',{value:{gpu},configurable:true});
globalThis.devicePixelRatio=1;
const width=640,height=640;
let target,device;
const context={configure(options){device=options.device;target=device.createTexture({size:[width,height],format:options.format,usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});},getCurrentTexture(){return target;}};
const canvas={width,height,clientWidth:width,clientHeight:height,getContext:()=>context};
const errors=[];
const renderer=await Renderer.create(canvas,error=>errors.push(error.message));
const m=await createMelee({print:()=>{}}),disc=path.resolve(process.argv[2]||'native/build/simulator-test.ciso');
m.FS.mkdir('/disc');m.FS.mount(m.NODEFS,{root:path.dirname(disc)},'/disc');
const check=result=>assert.ok(result,m.UTF8ToString(m._web_error()));
check(m.ccall('web_open_disc','number',['string'],['/disc/'+path.basename(disc)]));
const report=[];
for(const fighter of [0,1,2]){
 check(m._web_load_fighter(fighter));
 device.pushErrorScope('validation');
 await renderer.load(snapshot(m,true).parts);
 const frames=[];
 for(const frame of [0,20]){
  check(m._web_step(frame));renderer.update(snapshot(m,false).parts);renderer.draw();
  const readback=device.createBuffer({size:width*height*4,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
  const encoder=device.createCommandEncoder();encoder.copyTextureToBuffer({texture:target},{buffer:readback,bytesPerRow:width*4},[width,height]);device.queue.submit([encoder.finish()]);
  await readback.mapAsync(GPUMapMode.READ);const pixels=new Uint8Array(readback.getMappedRange()).slice();readback.unmap();readback.destroy();
  let colored=0;const rgb=new Uint8Array(width*height*3);
  const bg=pixels.slice(0,3);
  for(let i=0;i<width*height;i++){
   if(Math.abs(pixels[i*4]-bg[0])+Math.abs(pixels[i*4+1]-bg[1])+Math.abs(pixels[i*4+2]-bg[2])>30)colored++;
   const bgra=renderer.format==='bgra8unorm';rgb[i*3]=pixels[i*4+(bgra?2:0)];rgb[i*3+1]=pixels[i*4+1];rgb[i*3+2]=pixels[i*4+(bgra?0:2)];
  }
  assert.ok(colored>1500,`Expected visible fighter; got ${colored} foreground pixels`);
  await writeFile(`native/build/web/fighter-${fighter}-${frame}.ppm`,Buffer.concat([Buffer.from(`P6\n${width} ${height}\n255\n`),rgb]));
  frames.push({frame,colored,hash:createHash('sha256').update(pixels).digest('hex')});
 }
 assert.notEqual(frames[0].hash,frames[1].hash);
 assert.equal(await device.popErrorScope(),null);report.push({fighter,frames});
}
assert.deepEqual(errors,[]);renderer.clear();renderer.depth.destroy();renderer.uniform.destroy();target.destroy();m._web_close_scene();device.destroy();
console.log(JSON.stringify(report,null,2));
// Dawn's Node wrapper owns native event-loop handles until collection/process exit.
process.exit(0);
