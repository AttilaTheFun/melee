// Explicit import metadata for the Emscripten 6.0.9 Emdawn port. JS GPU
// objects live only on the browser thread; worker calls proxy there. The main
// event loop stays free to resolve asynchronous GPU promises.
// allowMissing permits metadata before Emscripten loads the port library.
const meleeGPUImports = [
  "emscripten_webgpu_get_device",
  "emwgpuAdapterRequestDevice",
  "emwgpuBufferDestroy",
  "emwgpuBufferGetConstMappedRange",
  "emwgpuBufferGetMappedRange",
  "emwgpuBufferMapAsync",
  "emwgpuBufferReadMappedRange",
  "emwgpuBufferUnmap",
  "emwgpuBufferWriteMappedRange",
  "emwgpuDelete",
  "emwgpuDeviceCreateBuffer",
  "emwgpuDeviceCreateComputePipelineAsync",
  "emwgpuDeviceCreateRenderPipelineAsync",
  "emwgpuDeviceCreateShaderModule",
  "emwgpuDeviceDestroy",
  "emwgpuDevicePopErrorScope",
  "emwgpuGetPreferredFormat",
  "emwgpuInstanceRequestAdapter",
  "emwgpuQueueOnSubmittedWorkDone",
  "emwgpuSetLabel",
  "emwgpuShaderModuleGetCompilationInfo",
  "emwgpuWaitAny",
  "wgpuAdapterGetFeatures",
  "wgpuAdapterGetInfo",
  "wgpuAdapterGetLimits",
  "wgpuAdapterHasFeature",
  "wgpuBufferGetSize",
  "wgpuBufferGetUsage",
  "wgpuCommandEncoderBeginComputePass",
  "wgpuCommandEncoderBeginRenderPass",
  "wgpuCommandEncoderClearBuffer",
  "wgpuCommandEncoderCopyBufferToBuffer",
  "wgpuCommandEncoderCopyBufferToTexture",
  "wgpuCommandEncoderCopyTextureToBuffer",
  "wgpuCommandEncoderCopyTextureToTexture",
  "wgpuCommandEncoderFinish",
  "wgpuCommandEncoderInsertDebugMarker",
  "wgpuCommandEncoderPopDebugGroup",
  "wgpuCommandEncoderPushDebugGroup",
  "wgpuCommandEncoderResolveQuerySet",
  "wgpuCommandEncoderWriteTimestamp",
  "wgpuComputePassEncoderDispatchWorkgroups",
  "wgpuComputePassEncoderDispatchWorkgroupsIndirect",
  "wgpuComputePassEncoderEnd",
  "wgpuComputePassEncoderInsertDebugMarker",
  "wgpuComputePassEncoderPopDebugGroup",
  "wgpuComputePassEncoderPushDebugGroup",
  "wgpuComputePassEncoderSetBindGroup",
  "wgpuComputePassEncoderSetPipeline",
  "wgpuComputePassEncoderWriteTimestamp",
  "wgpuComputePipelineGetBindGroupLayout",
  "wgpuDeviceCreateBindGroup",
  "wgpuDeviceCreateBindGroupLayout",
  "wgpuDeviceCreateCommandEncoder",
  "wgpuDeviceCreateComputePipeline",
  "wgpuDeviceCreatePipelineLayout",
  "wgpuDeviceCreateQuerySet",
  "wgpuDeviceCreateRenderBundleEncoder",
  "wgpuDeviceCreateRenderPipeline",
  "wgpuDeviceCreateSampler",
  "wgpuDeviceCreateTexture",
  "wgpuDeviceGetAdapterInfo",
  "wgpuDeviceGetFeatures",
  "wgpuDeviceGetLimits",
  "wgpuDeviceHasFeature",
  "wgpuDevicePushErrorScope",
  "wgpuGetProcAddress",
  "wgpuInstanceCreateSurface",
  "wgpuInstanceGetWGSLLanguageFeatures",
  "wgpuInstanceHasWGSLLanguageFeature",
  "wgpuQuerySetDestroy",
  "wgpuQuerySetGetCount",
  "wgpuQuerySetGetType",
  "wgpuQueueSubmit",
  "wgpuQueueWriteBuffer",
  "wgpuQueueWriteTexture",
  "wgpuRenderBundleEncoderDraw",
  "wgpuRenderBundleEncoderDrawIndexed",
  "wgpuRenderBundleEncoderDrawIndexedIndirect",
  "wgpuRenderBundleEncoderDrawIndirect",
  "wgpuRenderBundleEncoderFinish",
  "wgpuRenderBundleEncoderInsertDebugMarker",
  "wgpuRenderBundleEncoderPopDebugGroup",
  "wgpuRenderBundleEncoderPushDebugGroup",
  "wgpuRenderBundleEncoderSetBindGroup",
  "wgpuRenderBundleEncoderSetIndexBuffer",
  "wgpuRenderBundleEncoderSetPipeline",
  "wgpuRenderBundleEncoderSetVertexBuffer",
  "wgpuRenderPassEncoderBeginOcclusionQuery",
  "wgpuRenderPassEncoderDraw",
  "wgpuRenderPassEncoderDrawIndexed",
  "wgpuRenderPassEncoderDrawIndexedIndirect",
  "wgpuRenderPassEncoderDrawIndirect",
  "wgpuRenderPassEncoderEnd",
  "wgpuRenderPassEncoderEndOcclusionQuery",
  "wgpuRenderPassEncoderExecuteBundles",
  "wgpuRenderPassEncoderInsertDebugMarker",
  "wgpuRenderPassEncoderMultiDrawIndexedIndirect",
  "wgpuRenderPassEncoderMultiDrawIndirect",
  "wgpuRenderPassEncoderPopDebugGroup",
  "wgpuRenderPassEncoderPushDebugGroup",
  "wgpuRenderPassEncoderSetBindGroup",
  "wgpuRenderPassEncoderSetBlendConstant",
  "wgpuRenderPassEncoderSetIndexBuffer",
  "wgpuRenderPassEncoderSetPipeline",
  "wgpuRenderPassEncoderSetScissorRect",
  "wgpuRenderPassEncoderSetStencilReference",
  "wgpuRenderPassEncoderSetVertexBuffer",
  "wgpuRenderPassEncoderSetViewport",
  "wgpuRenderPassEncoderWriteTimestamp",
  "wgpuRenderPipelineGetBindGroupLayout",
  "wgpuSurfaceConfigure",
  "wgpuSurfaceGetCurrentTexture",
  "wgpuSurfacePresent",
  "wgpuSurfaceUnconfigure",
  "wgpuTextureCreateView",
  "wgpuTextureDestroy",
  "wgpuTextureGetDepthOrArrayLayers",
  "wgpuTextureGetDimension",
  "wgpuTextureGetFormat",
  "wgpuTextureGetHeight",
  "wgpuTextureGetMipLevelCount",
  "wgpuTextureGetSampleCount",
  "wgpuTextureGetTextureBindingViewDimension",
  "wgpuTextureGetUsage",
  "wgpuTextureGetWidth"
];
const meleeGPUProxyMetadata = {};
for (const name of meleeGPUImports) meleeGPUProxyMetadata[name + '__proxy'] = 'sync';
// These void calls carry only scalar values and owned resource handles.
// Emscripten 6.0.9 copies scalar arguments into its FIFO system proxy queue.
// End() remains synchronous on that same queue, so it drains these commands
// before the render worker can release the pass. Resource deletion also stays
// synchronous on that queue, draining prior uses before a handle is freed.
// Never extend this list to
// functions with pointers to temporary Wasm data (descriptors/offset arrays).
for (const name of [
  'wgpuRenderPassEncoderDraw', 'wgpuRenderPassEncoderDrawIndexed',
  'wgpuRenderPassEncoderSetViewport', 'wgpuRenderPassEncoderSetScissorRect',
  'wgpuRenderPassEncoderSetPipeline', 'wgpuRenderPassEncoderSetIndexBuffer',
  'wgpuRenderPassEncoderSetVertexBuffer',
]) meleeGPUProxyMetadata[name + '__proxy'] = 'async';
addToLibrary(meleeGPUProxyMetadata, {allowMissing: true});

// Opt-in resource-lifetime diagnostics for browser-only integration tests.
addToLibrary({
  melee_browser_gpu_debug__proxy:'sync',
  melee_browser_gpu_debug__sig:'v',
  melee_browser_gpu_debug__deps:['$WebGPU'],
  melee_browser_gpu_debug: () => {
    if(!Module.debugGPU)return;
    const history=[];
    const record=(action,ptr,value)=>{
      history.push({action,ptr,type:value?.constructor?.name,label:value?.label});
      if(history.length>2048)history.shift();
    };
    WebGPU.Internals.jsObjects=new Proxy(WebGPU.Internals.jsObjects,{
      set(target,ptr,value){record('create',ptr,value);target[ptr]=value;return true;},
      deleteProperty(target,ptr){record('delete',ptr,target[ptr]);return delete target[ptr];},
    });
    const original=WebGPU.getJsObject;
    WebGPU.getJsObject=ptr=>{
      if(ptr && !(ptr in WebGPU.Internals.jsObjects)){
        console.error('MISSING GPU HANDLE',ptr,JSON.stringify(history.filter(item=>Number(item.ptr)===ptr)));
      }
      return original(ptr);
    };
  },
});

// GX's two dynamic uniform offsets are values, not a Wasm pointer. End-of-pass
// and resource deletion retain their synchronous barriers on the same queue.
addToLibrary({
  melee_browser_bind_uniforms__proxy:'async',
  melee_browser_bind_uniforms__sig:'viiii',
  melee_browser_bind_uniforms__deps:['$WebGPU'],
  melee_browser_bind_uniforms: (pass, group, uniformOffset, immediateOffset) => {
    WebGPU.getJsObject(pass).setBindGroup(1,WebGPU.getJsObject(group),
      [uniformOffset>>>0,immediateOffset>>>0]);
  },
});

// No-offset bindings carry only scalar handles. The ordinary pointer-taking
// SetBindGroup import remains synchronous for callers with dynamic arrays.
addToLibrary({
  melee_browser_bind_group__proxy:'async',
  melee_browser_bind_group__sig:'viii',
  melee_browser_bind_group__deps:['$WebGPU'],
  melee_browser_bind_group: (pass, index, group) => {
    WebGPU.getJsObject(pass).setBindGroup(index>>>0,WebGPU.getJsObject(group));
  },
});
