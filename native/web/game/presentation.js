// Runs as one main-thread call: no CPU frame copy and no canvas texture retained
// across browser event-loop turns. Emdawn object lookup is pinned to 6.0.9.
addToLibrary({
  melee_browser_present__proxy: 'sync',
  melee_browser_present__sig: 'vppi',
  melee_browser_present__deps: ['$WebGPU'],
  melee_browser_present: (devicePtr, texturePtr, black) => {
    const device=WebGPU.getJsObject(devicePtr);
    let present=Module.meleePresentation;
    if(!present){
      const context=document.querySelector('#canvas').getContext('webgpu');
      const format=navigator.gpu.getPreferredCanvasFormat();
      context.configure({device,format,alphaMode:'opaque'});
      const module=device.createShaderModule({code:`
        struct Output { @builtin(position) position:vec4f, @location(0) uv:vec2f };
        @vertex fn vs(@builtin(vertex_index) index:u32)->Output {
          let uv=vec2f(f32((index<<1u)&2u),f32(index&2u));
          var out:Output;out.position=vec4f(uv*vec2f(2,-2)+vec2f(-1,1),0,1);out.uv=uv;return out;
        }
        @group(0) @binding(0) var image:texture_2d<f32>;
        @group(0) @binding(1) var imageSampler:sampler;
        @fragment fn fs(in:Output)->@location(0) vec4f {
          return vec4f(textureSample(image,imageSampler,in.uv).rgb,1);
        }`});
      const pipeline=device.createRenderPipeline({layout:'auto',vertex:{module,entryPoint:'vs'},
        fragment:{module,entryPoint:'fs',targets:[{format}]},primitive:{topology:'triangle-list'}});
      present=Module.meleePresentation={context,pipeline,sampler:device.createSampler({magFilter:'linear',minFilter:'linear'}),frames:0};
    }
    const encoder=device.createCommandEncoder();
    const pass=encoder.beginRenderPass({colorAttachments:[{view:present.context.getCurrentTexture().createView(),
      clearValue:{r:0,g:0,b:0,a:1},loadOp:'clear',storeOp:'store'}]});
    if(!black && texturePtr){
      const image=WebGPU.getJsObject(texturePtr);
      const group=device.createBindGroup({layout:present.pipeline.getBindGroupLayout(0),entries:[
        {binding:0,resource:image.createView()},{binding:1,resource:present.sampler}]});
      pass.setPipeline(present.pipeline);pass.setBindGroup(0,group);pass.draw(3);
    }
    pass.end();device.queue.submit([encoder.finish()]);present.frames++;
  },
});
