import Foundation
import Metal
import simd

struct GeometryVertex {
    var position: SIMD4<Float>
    var normal: SIMD4<Float>
    var color: SIMD4<Float>
    var uv: SIMD4<Float>
}
struct GeometryTexture {
    let texture: MTLTexture
    let sampler: MTLSamplerState
}
// GX register values, also used verbatim by the fragment shader. The standalone
// diagnostic default writes alpha; real materials use the original HSD snapshot.
struct GeometryPixelState: Hashable {
    var colorUpdate: UInt32 = 1, alphaUpdate: UInt32 = 1, dstAlphaEnable: UInt32 = 0, dstAlpha: UInt32 = 0
    var blendType: UInt32 = 0, srcFactor: UInt32 = 4, dstFactor: UInt32 = 5, logicOp: UInt32 = 15
    var zEnable: UInt32 = 1, zCompare: UInt32 = 3, zUpdate: UInt32 = 1, beforeTexture: UInt32 = 0
    var alphaCompare0: UInt32 = 7, alphaRef0: UInt32 = 0, alphaOperation: UInt32 = 0
    var alphaCompare1: UInt32 = 7, alphaRef1: UInt32 = 0, dither: UInt32 = 0
}
struct GeometryDraw {
    var type: MTLPrimitiveType
    var first: Int
    var count: Int
    var texture: GeometryTexture? = nil
    var colorOperation: Float = 0
    var alphaOperation: Float = 0
    var blend: Float = 0
    var lodBias: Float = 0
    var pixel = GeometryPixelState()
}
enum GeometryRenderError: Error { case unavailable, allocation, command(String) }

// Reusable offscreen geometry pass. This is not a GX material/TEV renderer yet.
final class MetalGeometryRenderer {
    let device: MTLDevice
    private let queue: MTLCommandQueue
    private let library: MTLLibrary
    private var pipelines = [GeometryPixelState: MTLRenderPipelineState]()
    private var depthStates = [GeometryPixelState: MTLDepthStencilState]()
    private var whiteTexture: GeometryTexture!

    init() throws {
        guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
            throw GeometryRenderError.unavailable
        }
        self.device = device; self.queue = queue
        let source = """
        #include <metal_stdlib>
        using namespace metal;
        struct Vertex { float4 position; float4 normal; float4 color; float4 uv; };
        struct Raster { float4 position [[position]]; float3 normal; float4 color; float2 uv; float size [[point_size]]; };
        struct PixelState {
            uint colorUpdate, alphaUpdate, dstAlphaEnable, dstAlpha;
            uint blendType, srcFactor, dstFactor, logicOp;
            uint zEnable, zCompare, zUpdate, beforeTexture;
            uint alphaCompare0, alphaRef0, alphaOperation, alphaCompare1, alphaRef1, dither;
        };
        bool alpha_compare(uint a, uint reference, uint comparison) {
            switch(comparison) {
                case 0: return false;
                case 1: return a < reference;
                case 2: return a == reference;
                case 3: return a <= reference;
                case 4: return a > reference;
                case 5: return a != reference;
                case 6: return a >= reference;
                default: return true;
            }
        }
        vertex Raster geometry_vertex(uint id [[vertex_id]], device const Vertex* v [[buffer(0)]],
                                      constant float4x4& matrix [[buffer(1)]]) {
            Raster out; out.position = matrix * v[id].position;
            out.normal = v[id].normal.xyz; out.color = v[id].color; out.uv = v[id].uv.xy; out.size = 1;
            return out;
        }
        float4 shade_fragment(Raster in, texture2d<float> tex, sampler smp,
                              constant float4& settings, constant PixelState& state) {
            float4 c = in.color, t = tex.sample(smp, in.uv, bias(settings.w));
            switch(uint(settings.x)) {
                case 1: c.rgb = mix(c.rgb,t.rgb,t.a); break;
                case 2: c.rgb = mix(c.rgb,t.rgb,t.rgb); break;
                case 3: c.rgb = mix(c.rgb,t.rgb,settings.z); break;
                case 4: c.rgb *= t.rgb; break;
                case 5: c.rgb = t.rgb; break;
                case 7: c.rgb += t.rgb; break;
                case 8: c.rgb -= t.rgb; break;
            }
            switch(uint(settings.y)) {
                case 1: c.a = mix(c.a,t.a,t.a); break;
                case 2: c.a = mix(c.a,t.a,settings.z); break;
                case 3: c.a *= t.a; break;
                case 4: c.a = t.a; break;
                case 6: c.a += t.a; break;
                case 7: c.a -= t.a; break;
            }
            c = clamp(c,0.0,1.0);
            uint alpha = uint(round(c.a * 255.0));
            bool a = alpha_compare(alpha,state.alphaRef0,state.alphaCompare0);
            bool b = alpha_compare(alpha,state.alphaRef1,state.alphaCompare1);
            bool pass = state.alphaOperation == 0 ? a && b : state.alphaOperation == 1 ? a || b :
                        state.alphaOperation == 2 ? a != b : a == b;
            if(!pass) discard_fragment();
            float len = length(in.normal);
            float3 n = len > 1e-8 ? in.normal / len : float3(0,0,1);
            float shade = 0.25 + 0.75 * max(dot(n, normalize(float3(0.4,0.8,0.6))), 0.0);
            return float4(c.rgb * shade, c.a);
        }
        fragment float4 geometry_fragment(Raster in [[stage_in]], texture2d<float> tex [[texture(0)]],
            sampler smp [[sampler(0)]], constant float4& settings [[buffer(0)]], constant PixelState& state [[buffer(1)]]) {
            return shade_fragment(in,tex,smp,settings,state);
        }
        [[early_fragment_tests]] fragment float4 geometry_fragment_early(Raster in [[stage_in]], texture2d<float> tex [[texture(0)]],
            sampler smp [[sampler(0)]], constant float4& settings [[buffer(0)]], constant PixelState& state [[buffer(1)]]) {
            return shade_fragment(in,tex,smp,settings,state);
        }
        """
        library = try device.makeLibrary(source: source, options: nil)
        whiteTexture = try upload(width: 1,height: 1,levels: [Data([255,255,255,255])],
            wrapS: 0,wrapT: 0,mag: 1,min: 1,minLOD: 0,maxLOD: 0,anisotropy: 0)
    }

    private func states(_ state: GeometryPixelState) throws -> (MTLRenderPipelineState,MTLDepthStencilState) {
        guard state.blendType <= 3, state.srcFactor <= 7, state.dstFactor <= 7, state.logicOp <= 15,
              state.zCompare <= 7, state.alphaCompare0 <= 7, state.alphaCompare1 <= 7,
              state.alphaOperation <= 3, state.alphaRef0 <= 255, state.alphaRef1 <= 255 else {
            throw GeometryRenderError.command("Invalid GX pixel state")
        }
        guard state.blendType != 2, state.dstAlphaEnable == 0, state.dither == 0 else {
            throw GeometryRenderError.command("GX logic operations, destination-alpha override and EFB dithering are not implemented")
        }
        // Alpha comparisons/references are dynamic shader inputs. Animating
        // them must not allocate new render pipelines every frame.
        var key = state
        key.alphaCompare0 = 0;key.alphaRef0 = 0;key.alphaOperation = 0
        key.alphaCompare1 = 0;key.alphaRef1 = 0;key.dstAlpha = 0;key.logicOp = 0
        if key.blendType != 1 { key.srcFactor = 0;key.dstFactor = 0 }
        if let pipeline = pipelines[key], let depth = depthStates[key] { return (pipeline,depth) }
        let descriptor = MTLRenderPipelineDescriptor()
        descriptor.vertexFunction = library.makeFunction(name: "geometry_vertex")
        descriptor.fragmentFunction = library.makeFunction(name: state.beforeTexture != 0 ? "geometry_fragment_early" : "geometry_fragment")
        let color = descriptor.colorAttachments[0]!
        color.pixelFormat = .rgba8Unorm
        color.writeMask = []
        if state.colorUpdate != 0 { color.writeMask.formUnion([.red,.green,.blue]) }
        if state.alphaUpdate != 0 { color.writeMask.insert(.alpha) }
        color.isBlendingEnabled = state.blendType == 1 || state.blendType == 3
        if state.blendType == 3 {
            color.rgbBlendOperation = .reverseSubtract; color.alphaBlendOperation = .reverseSubtract
            color.sourceRGBBlendFactor = .one; color.destinationRGBBlendFactor = .one
            color.sourceAlphaBlendFactor = .one; color.destinationAlphaBlendFactor = .one
        } else {
            // GX value 2/3 means destination color in the source factor, but
            // source color in the destination factor. Alpha uses its A channel.
            let src: [MTLBlendFactor] = [.zero,.one,.destinationColor,.oneMinusDestinationColor,.sourceAlpha,.oneMinusSourceAlpha,.destinationAlpha,.oneMinusDestinationAlpha]
            let dst: [MTLBlendFactor] = [.zero,.one,.sourceColor,.oneMinusSourceColor,.sourceAlpha,.oneMinusSourceAlpha,.destinationAlpha,.oneMinusDestinationAlpha]
            let srcA: [MTLBlendFactor] = [.zero,.one,.destinationAlpha,.oneMinusDestinationAlpha,.sourceAlpha,.oneMinusSourceAlpha,.destinationAlpha,.oneMinusDestinationAlpha]
            let dstA: [MTLBlendFactor] = [.zero,.one,.sourceAlpha,.oneMinusSourceAlpha,.sourceAlpha,.oneMinusSourceAlpha,.destinationAlpha,.oneMinusDestinationAlpha]
            color.sourceRGBBlendFactor = src[Int(state.srcFactor)];color.destinationRGBBlendFactor = dst[Int(state.dstFactor)]
            color.sourceAlphaBlendFactor = srcA[Int(state.srcFactor)];color.destinationAlphaBlendFactor = dstA[Int(state.dstFactor)]
        }
        descriptor.depthAttachmentPixelFormat = .depth32Float
        let pipeline = try device.makeRenderPipelineState(descriptor: descriptor)
        let depth = MTLDepthStencilDescriptor()
        let comparisons: [MTLCompareFunction] = [.never,.less,.equal,.lessEqual,.greater,.notEqual,.greaterEqual,.always]
        depth.depthCompareFunction = state.zEnable != 0 ? comparisons[Int(state.zCompare)] : .always
        depth.isDepthWriteEnabled = state.zEnable != 0 && state.zUpdate != 0
        guard let depthState = device.makeDepthStencilState(descriptor: depth) else { throw GeometryRenderError.allocation }
        pipelines[key] = pipeline;depthStates[key] = depthState
        return (pipeline,depthState)
    }

    func upload(width: Int,height: Int,levels: [Data],wrapS: UInt32,wrapT: UInt32,
                mag: UInt32,min: UInt32,minLOD: Float,maxLOD: Float,anisotropy: UInt32) throws -> GeometryTexture {
        guard width > 0,height > 0,!levels.isEmpty,wrapS <= 2,wrapT <= 2,mag <= 1,min <= 5,anisotropy <= 2 else {
            throw GeometryRenderError.allocation
        }
        let desc = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm,width: width,height: height,mipmapped: levels.count > 1)
        guard levels.count <= desc.mipmapLevelCount else { throw GeometryRenderError.allocation }
        desc.mipmapLevelCount = levels.count; desc.storageMode = .shared; desc.usage = [.shaderRead]
        guard let texture = device.makeTexture(descriptor: desc) else { throw GeometryRenderError.allocation }
        for (level,data) in levels.enumerated() {
            let w = max(1,width >> level), h = max(1,height >> level)
            guard data.count == w*h*4 else { throw GeometryRenderError.allocation }
            data.withUnsafeBytes { texture.replace(region: MTLRegionMake2D(0,0,w,h),mipmapLevel: level,withBytes: $0.baseAddress!,bytesPerRow: w*4) }
        }
        let wrap: [MTLSamplerAddressMode] = [.clampToEdge,.repeat,.mirrorRepeat]
        let sampler = MTLSamplerDescriptor();sampler.sAddressMode = wrap[Int(wrapS)];sampler.tAddressMode = wrap[Int(wrapT)]
        sampler.magFilter = mag == 0 ? .nearest : .linear; sampler.minFilter = min % 2 == 0 ? .nearest : .linear
        sampler.mipFilter = min < 2 ? .notMipmapped : min < 4 ? .nearest : .linear
        sampler.lodMinClamp = minLOD;sampler.lodMaxClamp = maxLOD;sampler.maxAnisotropy = 1 << anisotropy
        guard let state = device.makeSamplerState(descriptor: sampler) else { throw GeometryRenderError.allocation }
        return GeometryTexture(texture: texture,sampler: state)
    }

    func render(vertices: [GeometryVertex], draws: [GeometryDraw], width: Int, height: Int,
                matrix: simd_float4x4) throws -> [UInt8] {
        guard width > 0, height > 0, width <= 8192, height <= 8192,
              !vertices.isEmpty, MemoryLayout<GeometryVertex>.stride == 64,
              MemoryLayout<GeometryPixelState>.stride == 72 else { throw GeometryRenderError.allocation }
        var renderStates = [(MTLRenderPipelineState,MTLDepthStencilState)]()
        for draw in draws {
            guard draw.first >= 0, draw.count >= 0, draw.first <= vertices.count,
                  draw.count <= vertices.count - draw.first else { throw GeometryRenderError.allocation }
            renderStates.append(try states(draw.pixel))
        }
        let textureDesc = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm,
            width: width, height: height, mipmapped: false)
        textureDesc.usage = [.renderTarget]; textureDesc.storageMode = .shared
        let depthDesc = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .depth32Float,
            width: width, height: height, mipmapped: false)
        depthDesc.usage = [.renderTarget]; depthDesc.storageMode = .private
        guard let target = device.makeTexture(descriptor: textureDesc),
              let depth = device.makeTexture(descriptor: depthDesc),
              let buffer = vertices.withUnsafeBytes({ device.makeBuffer(bytes: $0.baseAddress!, length: $0.count) }),
              let command = queue.makeCommandBuffer() else { throw GeometryRenderError.allocation }
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = target; pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = MTLClearColor(red: 0.035, green: 0.045, blue: 0.065, alpha: 1)
        pass.depthAttachment.texture = depth; pass.depthAttachment.loadAction = .clear
        pass.depthAttachment.storeAction = .dontCare; pass.depthAttachment.clearDepth = 1
        guard let encoder = command.makeRenderCommandEncoder(descriptor: pass) else { throw GeometryRenderError.allocation }
        encoder.setCullMode(.none)
        encoder.setVertexBuffer(buffer, offset: 0, index: 0)
        var transform = matrix
        encoder.setVertexBytes(&transform, length: MemoryLayout<simd_float4x4>.stride, index: 1)
        for (index,draw) in draws.enumerated() where draw.count > 0 {
            encoder.setRenderPipelineState(renderStates[index].0);encoder.setDepthStencilState(renderStates[index].1)
            let texture = draw.texture ?? whiteTexture!
            encoder.setFragmentTexture(texture.texture,index: 0);encoder.setFragmentSamplerState(texture.sampler,index: 0)
            var settings = SIMD4(draw.colorOperation,draw.alphaOperation,draw.blend,draw.lodBias)
            encoder.setFragmentBytes(&settings,length: MemoryLayout<SIMD4<Float>>.stride,index: 0)
            var pixel = draw.pixel
            encoder.setFragmentBytes(&pixel,length: MemoryLayout<GeometryPixelState>.stride,index: 1)
            encoder.drawPrimitives(type: draw.type, vertexStart: draw.first, vertexCount: draw.count)
        }
        encoder.endEncoding(); command.commit(); command.waitUntilCompleted()
        guard command.status == .completed else { throw GeometryRenderError.command(command.error?.localizedDescription ?? "Metal command failed") }
        var pixels = [UInt8](repeating: 0, count: width * height * 4)
        pixels.withUnsafeMutableBytes { raw in
            target.getBytes(raw.baseAddress!, bytesPerRow: width * 4,
                from: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0)
        }
        return pixels
    }
}
