import Foundation
import Metal
import simd

// Deliberately uses synthetic pixels: these checks need no game image and open
// no windows. They test this preview's supported operations, not full GX TEV.
@main struct TestMetalRenderer {
    static func main() throws {
        let renderer = try MetalGeometryRenderer()
        let normal = simd_normalize(SIMD3<Float>(0.4,0.8,0.6))
        let base = SIMD4<Float>(0.4,0.6,0.8,0.7)
        func pixel(_ texture: GeometryTexture?, _ color: UInt32 = 5, _ alpha: UInt32 = 4,
                   uv: SIMD2<Float> = SIMD2(0.25,0.25), bias: Float = 0) throws -> [UInt8] {
            let vertices = [SIMD4<Float>(-1,-1,0.5,1), SIMD4<Float>(3,-1,0.5,1), SIMD4<Float>(-1,3,0.5,1)].map {
                GeometryVertex(position: $0, normal: SIMD4(normal,0), color: base, uv: SIMD4(uv.x,uv.y,0,0))
            }
            let draw = GeometryDraw(type: .triangle,first: 0,count: 3,texture: texture,
                colorOperation: Float(color),alphaOperation: Float(alpha),blend: 0.3,lodBias: bias)
            return Array(try renderer.render(vertices: vertices,draws: [draw],width: 4,height: 4,
                matrix: matrix_identity_float4x4)[40..<44])
        }
        func check(_ actual: [UInt8], _ expected: SIMD4<Float>, _ label: String) {
            for channel in 0..<4 {
                let reference = Int((max(0,min(1,expected[channel]))*255).rounded())
                precondition(abs(Int(actual[channel])-reference) <= 2,
                    "\(label), channel \(channel): got \(actual), expected \(expected)")
            }
        }
        let texel = SIMD4<Float>(64/255.0,128/255.0,192/255.0,96/255.0)
        let texture = try renderer.upload(width: 1,height: 1,levels: [Data([64,128,192,96])],
            wrapS: 0,wrapT: 0,mag: 0,min: 0,minLOD: 0,maxLOD: 0,anisotropy: 0)
        for operation: UInt32 in 0...8 {
            var expected = base
            for c in 0..<3 {
                switch operation {
                case 1: expected[c] = base[c]*(1-texel.w)+texel[c]*texel.w
                case 2: expected[c] = base[c]*(1-texel[c])+texel[c]*texel[c]
                case 3: expected[c] = base[c]*0.7+texel[c]*0.3
                case 4: expected[c] = base[c]*texel[c]
                case 5: expected[c] = texel[c]
                case 7: expected[c] = base[c]+texel[c]
                case 8: expected[c] = base[c]-texel[c]
                default: break
                }
            }
            check(try pixel(texture,operation,0),expected,"color operation \(operation)")
        }
        for operation: UInt32 in 0...7 {
            var expected = base
            switch operation {
            case 1: expected.w = base.w*(1-texel.w)+texel.w*texel.w
            case 2: expected.w = base.w*0.7+texel.w*0.3
            case 3: expected.w = base.w*texel.w
            case 4: expected.w = texel.w
            case 6: expected.w = base.w+texel.w
            case 7: expected.w = base.w-texel.w
            default: break
            }
            check(try pixel(texture,0,operation),expected,"alpha operation \(operation)")
        }
        check(try pixel(nil,0,0),base,"untextured fallback")
        let corners = Data([255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255])
        for wrap: UInt32 in 0...2 {
            let t = try renderer.upload(width: 2,height: 2,levels: [corners],wrapS: wrap,wrapT: wrap,
                mag: 0,min: 0,minLOD: 0,maxLOD: 0,anisotropy: 0)
            check(try pixel(t,uv: SIMD2(0.25,0.25)),SIMD4(1,0,0,1),"top-left texel")
            check(try pixel(t,uv: SIMD2(0.75,0.75)),SIMD4(1,1,1,1),"bottom-right texel")
            check(try pixel(t,uv: SIMD2(1.25,0.25)),wrap == 1 ? SIMD4(1,0,0,1) : SIMD4(0,1,0,1),"wrap \(wrap)")
        }
        let linear = try renderer.upload(width: 2,height: 2,levels: [corners],wrapS: 0,wrapT: 0,
            mag: 1,min: 1,minLOD: 0,maxLOD: 0,anisotropy: 0)
        check(try pixel(linear,uv: SIMD2(0.5,0.5)),SIMD4(0.5,0.5,0.5,1),"linear sampling")
        let mip = try renderer.upload(width: 2,height: 2,levels: [corners,Data([32,64,128,255])],
            wrapS: 0,wrapT: 0,mag: 0,min: 2,minLOD: 1,maxLOD: 1,anisotropy: 0)
        check(try pixel(mip),SIMD4(32/255.0,64/255.0,128/255.0,1),"uploaded mip and LOD clamp")
        do {
            _ = try renderer.upload(width: 2,height: 2,levels: [Data([0])],wrapS: 0,wrapT: 0,
                mag: 0,min: 0,minLOD: 0,maxLOD: 0,anisotropy: 0)
            preconditionFailure("short upload accepted")
        } catch GeometryRenderError.allocation {}
        func layers(_ layers: [(SIMD4<Float>,Float,GeometryPixelState)]) throws -> [UInt8] {
            var vertices = [GeometryVertex](),draws = [GeometryDraw]()
            for (color,z,state) in layers {
                let first = vertices.count
                vertices += [SIMD4<Float>(-1,-1,z,1),SIMD4<Float>(3,-1,z,1),SIMD4<Float>(-1,3,z,1)].map {
                    GeometryVertex(position: $0,normal: SIMD4(normal,0),color: color,uv: .zero)
                }
                var draw = GeometryDraw(type: .triangle,first: first,count: 3)
                draw.pixel = state;draws.append(draw)
            }
            return Array(try renderer.render(vertices: vertices,draws: draws,width: 4,height: 4,
                matrix: matrix_identity_float4x4)[40..<44])
        }
        let clear = SIMD4<Float>(0.035,0.045,0.065,1), opaque = GeometryPixelState()
        let source = SIMD4<Float>(0.7,0.3,0.1,0.4), destination = SIMD4<Float>(0.2,0.4,0.6,0.8)
        func comparison(_ a: Float,_ b: Float,_ op: UInt32) -> Bool {
            switch op {
            case 0: return false
            case 1: return a < b
            case 2: return a == b
            case 3: return a <= b
            case 4: return a > b
            case 5: return a != b
            case 6: return a >= b
            default: return true
            }
        }
        for compare: UInt32 in 0...7 {
            for alpha: UInt32 in 127...129 {
                var state = opaque;state.alphaCompare0 = compare;state.alphaRef0 = 128
                var c = source;c.w = Float(alpha)/255
                check(try layers([(c,0.5,state)]),comparison(Float(alpha),128,compare) ? c : clear,"alpha comparison \(compare)/\(alpha)")
            }
            for z: Float in [0.25,0.5,0.75] {
                var state = opaque;state.zCompare = compare
                check(try layers([(destination,0.5,opaque),(source,z,state)]),comparison(z,0.5,compare) ? source : destination,"depth comparison \(compare)/\(z)")
            }
        }
        for op: UInt32 in 0...3 {
            for a in [false,true] { for b in [false,true] {
                var state = opaque;state.alphaCompare0 = a ? 7 : 0;state.alphaCompare1 = b ? 7 : 0;state.alphaOperation = op
                let pass = op == 0 ? a && b : op == 1 ? a || b : op == 2 ? a != b : a == b
                check(try layers([(source,0.5,state)]),pass ? source : clear,"alpha boolean operation \(op)/\(a)/\(b)")
            } }
        }
        for sf: UInt32 in 0...7 { for df: UInt32 in 0...7 {
            var state = opaque;state.blendType = 1;state.srcFactor = sf;state.dstFactor = df
            let srcFactors: [SIMD4<Float>] = [.zero,.one,destination,.one-destination,
                SIMD4(repeating: source.w),SIMD4(repeating: 1-source.w),SIMD4(repeating: destination.w),SIMD4(repeating: 1-destination.w)]
            let dstFactors: [SIMD4<Float>] = [.zero,.one,source,.one-source,
                SIMD4(repeating: source.w),SIMD4(repeating: 1-source.w),SIMD4(repeating: destination.w),SIMD4(repeating: 1-destination.w)]
            check(try layers([(destination,0.8,opaque),(source,0.2,state)]),
                source*srcFactors[Int(sf)]+destination*dstFactors[Int(df)],"blend factors \(sf)/\(df)")
        } }
        var subtract = opaque;subtract.blendType = 3
        check(try layers([(destination,0.8,opaque),(source,0.2,subtract)]),destination-source,"reverse subtract")
        for mask: UInt32 in 0...3 {
            var state = opaque;state.colorUpdate = mask&1;state.alphaUpdate = (mask>>1)&1
            var expected = destination
            if mask&1 != 0 { expected.x = source.x;expected.y = source.y;expected.z = source.z }
            if mask&2 != 0 { expected.w = source.w }
            check(try layers([(destination,0.8,opaque),(source,0.2,state)]),expected,"color/alpha write mask \(mask)")
        }
        for early: UInt32 in 0...1 {
            var rejected = opaque;rejected.alphaCompare0 = 0;rejected.beforeTexture = early
            check(try layers([(source,0.2,rejected),(destination,0.8,opaque)]),early == 0 ? destination : clear,"depth write before/after alpha test \(early)")
        }
        for disableTest in [false,true] {
            var state = opaque
            if disableTest { state.zEnable = 0 } else { state.zUpdate = 0 }
            check(try layers([(source,0.2,state),(destination,0.8,opaque)]),destination,"disabled depth test/update \(disableTest)")
        }
        for unsupported: UInt32 in 0...2 {
            var state = opaque
            if unsupported == 0 { state.blendType = 2 }
            if unsupported == 1 { state.dstAlphaEnable = 1 }
            if unsupported == 2 { state.dither = 1 }
            do { _ = try layers([(source,0.5,state)]);preconditionFailure("unsupported PE accepted") }
            catch GeometryRenderError.command {}
        }
        print("Metal preview: texture sampling, alpha comparisons/logic, 64 blend-factor pairs, subtraction, write masks, depth comparisons and early/late depth passed on \(renderer.device.name)")
    }
}
