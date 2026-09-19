import Foundation
import Metal
import simd
import CoreGraphics
import ImageIO
import UniformTypeIdentifiers

func pixelState(_ material: MeleeMaterial) throws -> GeometryPixelState {
    var s = MeleeGXPixelState()
    let valid = withUnsafeBytes(of: material.pixel_engine) {
        melee_gx_pixel_material(material.render_mode,material.has_pixel_engine ? $0.bindMemory(to: UInt8.self).baseAddress : nil,&s)
    }
    guard valid else { throw GeometryRenderError.command("Invalid material pixel descriptor") }
    return GeometryPixelState(colorUpdate: s.color_update,alphaUpdate: s.alpha_update,
        dstAlphaEnable: s.dst_alpha_enable,dstAlpha: s.dst_alpha,blendType: s.blend_type,
        srcFactor: s.src_factor,dstFactor: s.dst_factor,logicOp: s.logic_op,
        zEnable: s.z_enable,zCompare: s.z_compare,zUpdate: s.z_update,beforeTexture: s.before_texture,
        alphaCompare0: s.alpha_compare0,alphaRef0: s.alpha_ref0,alphaOperation: s.alpha_operation,
        alphaCompare1: s.alpha_compare1,alphaRef1: s.alpha_ref1,dither: s.dither)
}

func viewProjection() -> simd_float4x4 {
    let eye = SIMD3<Float>(28, 18, 32), target = SIMD3<Float>(0, 8, 0)
    let f = simd_normalize(target-eye), r = simd_normalize(simd_cross(f, SIMD3<Float>(0,1,0)))
    let u = simd_cross(r,f)
    let view = simd_float4x4(columns: (
        SIMD4(r.x,u.x,-f.x,0), SIMD4(r.y,u.y,-f.y,0), SIMD4(r.z,u.z,-f.z,0),
        SIMD4(-simd_dot(r,eye),-simd_dot(u,eye),simd_dot(f,eye),1)))
    let scale: Float = 1/tan(Float.pi/8), near: Float = 0.1, far: Float = 100
    let projection = simd_float4x4(columns: (SIMD4(scale,0,0,0), SIMD4(0,scale,0,0),
        SIMD4(0,0,far/(near-far),-1), SIMD4(0,0,near*far/(near-far),0)))
    return projection * view
}

@main struct RenderModel {
    static func main() {
        do { try run() }
        catch { fputs("Render failed: \(error)\n", stderr); exit(1) }
    }
    static func run() throws {
        let args = CommandLine.arguments
        guard (args.count == 6 || args.count == 8), let frame = Float(args[4]), frame.isFinite, frame >= 0,
              melee_render_test_init(),
              let model = melee_render_test_load(args[1],args[2],args[3],frame) else {
            throw GeometryRenderError.command("Usage: render-model COSTUME JOINT_SYMBOL MOTION_BUNDLE FRAME OUTPUT.png [FIGHTER_DATA DATA_SYMBOL]")
        }
        defer { melee_render_test_release(model) }
        if args.count == 8 && !melee_render_test_fighter(model,args[6],args[7],frame) {
            throw GeometryRenderError.command("Fighter reset/model script preview failed (requires a supported base costume and matching first idle motion)")
        }
        let renderer = try MetalGeometryRenderer()
        var textureCache = [UInt32: GeometryTexture]()
        var pixelCache = [UInt32: GeometryPixelState]()
        var unsupportedLayers = Set<UInt32>()
        var vertices = [GeometryVertex](), draws = [GeometryDraw]()
        var parts = 0
        for partIndex in 0..<melee_model_part_count(model) {
            guard let part = melee_model_part(model,partIndex)?.pointee else { continue }
            if part.hidden { continue }
            parts += 1
            var selected: MeleeMaterialTexture? = nil
            var surfaceColor = SIMD4<Float>(0.68,0.74,0.83,1)
            if let material = part.material?.pointee {
                if pixelCache[material.offset] == nil { pixelCache[material.offset] = try pixelState(material) }
                let c = material.diffuse
                surfaceColor = SIMD4(Float(c.0)/255,Float(c.1)/255,Float(c.2)/255,material.alpha)
                for index in 0..<material.texture_count {
                    let t = material.textures[index]
                    if selected == nil && t.flags & 15 == 0 && t.source >= 4 && t.source <= 11 && t.mip_count > 0 &&
                       t.tev_active & 0xc0000000 == 0 && (t.flags >> 16) & 15 <= 8 && (t.flags >> 20) & 15 <= 7 {
                        selected = t
                        if textureCache[t.offset] == nil {
                            var levels = [Data]()
                            for mip in 0..<t.mip_count { let m = t.mips[mip]; levels.append(Data(bytes: m.rgba,count: m.size)) }
                            textureCache[t.offset] = try renderer.upload(width: Int(t.mips[0].width),height: Int(t.mips[0].height),levels: levels,
                                wrapS: t.wrap_s,wrapT: t.wrap_t,mag: t.mag_filter,min: t.effective_min_filter,minLOD: t.min_lod,maxLOD: t.max_lod,anisotropy: t.anisotropy)
                        }
                    } else { unsupportedLayers.insert(t.offset) }
                }
            }
            for drawIndex in 0..<part.draw_count {
                let draw = part.draws[drawIndex]
                var indices = [Int]()
                var primitive: MTLPrimitiveType = .triangle
                let first = Int(draw.first), count = Int(draw.count)
                switch draw.primitive {
                case 0x80:
                    var i = 0
                    while i+3 < count { indices += [i,i+1,i+2,i,i+2,i+3]; i += 4 }
                    if count-i == 3 { indices += [i,i+1,i+2] }
                case 0xa0:
                    if count >= 3 { for i in 2..<count { indices += [0,i-1,i] } }
                case 0x90: indices = Array(0..<(count/3*3))
                case 0x98: primitive = .triangleStrip; if count >= 3 { indices = Array(0..<count) }
                case 0xa8: primitive = .line; indices = Array(0..<(count/2*2))
                case 0xb0: primitive = .lineStrip; if count >= 2 { indices = Array(0..<count) }
                case 0xb8: primitive = .point; indices = Array(0..<count)
                default: throw GeometryRenderError.command("Unsupported primitive")
                }
                let start = vertices.count
                for index in indices {
                    guard first+index < part.vertex_count else { throw GeometryRenderError.command("Vertex range error") }
                    let v = part.vertices[first+index], n = v.normal.0
                    var uv = SIMD2<Float>(0,0)
                    if let t = selected {
                        let coordinates: [(Float,Float)] = [v.texcoord.0,v.texcoord.1,v.texcoord.2,v.texcoord.3,v.texcoord.4,v.texcoord.5,v.texcoord.6,v.texcoord.7]
                        let source = coordinates[Int(t.source)-4], x = t.matrix.0, y = t.matrix.1
                        uv = SIMD2(x.0*source.0+x.1*source.1+x.2+x.3,y.0*source.0+y.1*source.1+y.2+y.3)
                    }
                    var color = surfaceColor
                    if let material = part.material?.pointee, material.render_mode & 2 != 0 {
                        let c = v.color.0;color *= SIMD4(Float(c.0)/255,Float(c.1)/255,Float(c.2)/255,Float(c.3)/255)
                    }
                    vertices.append(GeometryVertex(position: SIMD4(v.position.0,v.position.1,v.position.2,1),
                        normal: SIMD4(n.0,n.1,n.2,0),color: color,uv: SIMD4(uv.x,uv.y,0,0)))
                }
                var outputDraw = GeometryDraw(type: primitive,first: start,count: vertices.count-start)
                if let material = part.material?.pointee { outputDraw.pixel = pixelCache[material.offset]! }
                if let t = selected {
                    outputDraw.texture = textureCache[t.offset];outputDraw.colorOperation = Float((t.flags >> 16)&15)
                    outputDraw.alphaOperation = Float((t.flags >> 20)&15);outputDraw.blend = t.blend;outputDraw.lodBias = t.lod_bias
                }
                draws.append(outputDraw)
            }
        }
        print("UV textures uploaded: \(textureCache.count); unsupported preview layers: \(unsupportedLayers.count)")
        print("Original HSD pixel setup: \(pixelCache.count) materials, \(Set(pixelCache.values).count) unique states")
        let width = 768, height = 768
        let pixels = try renderer.render(vertices: vertices, draws: draws, width: width, height: height, matrix: viewProjection())
        var changed = 0
        for i in stride(from: 0,to: pixels.count,by: 4) {
            if abs(Int(pixels[i])-9) > 2 || abs(Int(pixels[i+1])-11) > 2 || abs(Int(pixels[i+2])-17) > 2 { changed += 1 }
        }
        guard changed > 1000 else { throw GeometryRenderError.command("Render produced too few foreground pixels") }
        let data = Data(pixels) as CFData
        guard let provider = CGDataProvider(data: data),
              let image = CGImage(width: width,height: height,bitsPerComponent: 8,bitsPerPixel: 32,bytesPerRow: width*4,
                space: CGColorSpaceCreateDeviceRGB(),bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.last.rawValue),
                provider: provider,decode: nil,shouldInterpolate: false,intent: .defaultIntent),
              let destination = CGImageDestinationCreateWithURL(URL(fileURLWithPath: args[5]) as CFURL, UTType.png.identifier as CFString,1,nil) else {
            throw GeometryRenderError.allocation
        }
        CGImageDestinationAddImage(destination,image,nil)
        guard CGImageDestinationFinalize(destination) else { throw GeometryRenderError.command("PNG write failed") }
        print("Metal device: \(renderer.device.name); \(parts) parts, \(draws.count) draws, \(changed) foreground pixels; \(args[5])")
    }
}
