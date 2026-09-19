import Foundation
import Metal
import CoreGraphics
import ImageIO
import UniformTypeIdentifiers

@main struct MetalFXTest {
    static func render(_ pixels: Data, renderer: MeleeMetalFXRenderer, queue: MTLCommandQueue) throws -> Data {
        let count = renderer.width * renderer.height * 4
        let readback = renderer.device.makeBuffer(length: count, options: .storageModeShared)!
        let command = queue.makeCommandBuffer()!
        pixels.withUnsafeBytes { renderer.encode(rgba: $0.baseAddress!, into: command) }
        let blit = command.makeBlitCommandEncoder()!
        blit.copy(from: renderer.output, sourceSlice: 0, sourceLevel: 0,
            sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
            sourceSize: MTLSize(width: renderer.width, height: renderer.height, depth: 1),
            to: readback, destinationOffset: 0, destinationBytesPerRow: renderer.width * 4,
            destinationBytesPerImage: count)
        blit.endEncoding(); command.commit(); command.waitUntilCompleted()
        precondition(command.status == .completed, "MetalFX GPU error: \(String(describing: command.error))")
        return Data(bytes: readback.contents(), count: count)
    }
    static func main() throws {
        guard let device = MTLCreateSystemDefaultDevice(), MeleeMetalFXRenderer.supported,
              let queue = device.makeCommandQueue() else { fatalError("MetalFX unavailable on test Mac") }
        for scale in [2, 3] {
            let renderer = try MeleeMetalFXRenderer(device: device, scale: scale)
            // Distinct quadrants verify orientation and RGBA -> BGRA channels.
            var pattern = Data(count: 640 * 480 * 4)
            pattern.withUnsafeMutableBytes { raw in
                let p = raw.bindMemory(to: UInt8.self)
                for y in 0..<480 { for x in 0..<640 {
                    let i = (y * 640 + x) * 4
                    p[i] = x < 320 ? 255 : 0
                    p[i+1] = x >= 320 ? 255 : 0
                    p[i+2] = y >= 240 ? 255 : 0
                    p[i+3] = 255
                } }
            }
            let result = try render(pattern, renderer: renderer, queue: queue)
            for (x,y) in [(80,60),(560,60),(80,420),(560,420)] {
                let source = (y * 640 + x) * 4
                let output = (y * scale * renderer.width + x * scale) * 4
                for (a,b) in [(0,2),(1,1),(2,0)] {
                    precondition(abs(Int(result[output+a])-Int(pattern[source+b])) <= 4)
                }
            }
            // Reusing the scaler with fresh input must not retain the previous image.
            var dark = Data(repeating: 0, count: 640 * 480 * 4)
            for i in stride(from: 3, to: dark.count, by: 4) { dark[i] = 255 }
            let black = try render(dark, renderer: renderer, queue: queue)
            precondition(stride(from: 0, to: black.count, by: 4).allSatisfy { black[$0] < 3 && black[$0+1] < 3 && black[$0+2] < 3 })
            if CommandLine.arguments.count == 3 {
                let source = CGImageSourceCreateWithURL(URL(fileURLWithPath: CommandLine.arguments[1]) as CFURL, nil)!
                let image = CGImageSourceCreateImageAtIndex(source, 0, nil)!
                precondition(image.width == 640 && image.height == 480)
                var rgba = Data(count: 640 * 480 * 4)
                rgba.withUnsafeMutableBytes { raw in
                    let context = CGContext(data: raw.baseAddress, width: 640, height: 480, bitsPerComponent: 8,
                        bytesPerRow: 2560, space: CGColorSpaceCreateDeviceRGB(),
                        bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
                    context.draw(image, in: CGRect(x: 0, y: 0, width: 640, height: 480))
                }
                let pixels = try render(rgba, renderer: renderer, queue: queue)
                let provider = CGDataProvider(data: pixels as CFData)!
                let output = CGImage(width: renderer.width, height: renderer.height, bitsPerComponent: 8,
                    bitsPerPixel: 32, bytesPerRow: renderer.width * 4, space: CGColorSpaceCreateDeviceRGB(),
                    bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.noneSkipFirst.rawValue).union(.byteOrder32Little),
                    provider: provider, decode: nil, shouldInterpolate: false, intent: .defaultIntent)!
                let path = CommandLine.arguments[2] + "-\(scale)x.png"
                let writer = CGImageDestinationCreateWithURL(URL(fileURLWithPath: path) as CFURL, UTType.png.identifier as CFString, 1, nil)!
                CGImageDestinationAddImage(writer, output, nil); precondition(CGImageDestinationFinalize(writer))
            }
            print("MetalFX \(scale)×: dimensions, orientation, color channels, fresh-frame reuse and GPU completion passed")
        }
    }
}
