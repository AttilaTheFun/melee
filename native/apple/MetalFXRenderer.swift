import Foundation
import ImageIO
import UniformTypeIdentifiers
import Metal
#if canImport(MetalFX)
import MetalFX

/// Presentation-only spatial upscaling; the game's framebuffer stays 640×480.
/// The caller serializes access until the submitted command buffer completes.
final class MeleeMetalFXRenderer {
    enum Failure: LocalizedError {
        case unsupported, allocation
        var errorDescription: String? {
            switch self {
            case .unsupported: return "MetalFX is unavailable on this device."
            case .allocation: return "MetalFX could not allocate its image buffers."
            }
        }
    }
    static var supported: Bool {
        guard let device = MTLCreateSystemDefaultDevice() else { return false }
        return MTLFXSpatialScalerDescriptor.supportsDevice(device)
    }
    let device: MTLDevice
    let width: Int
    let height: Int
    private let scaler: MTLFXSpatialScaler
    private let input: MTLTexture
    let output: MTLTexture

    init(device: MTLDevice, scale: Int) throws {
        guard (2...3).contains(scale), MTLFXSpatialScalerDescriptor.supportsDevice(device) else {
            throw Failure.unsupported
        }
        self.device = device; width = 640 * scale; height = 480 * scale
        let descriptor = MTLFXSpatialScalerDescriptor()
        descriptor.inputWidth = 640; descriptor.inputHeight = 480
        descriptor.outputWidth = width; descriptor.outputHeight = height
        descriptor.colorTextureFormat = .rgba8Unorm
        descriptor.outputTextureFormat = .bgra8Unorm
        descriptor.colorProcessingMode = .perceptual
        guard let scaler = descriptor.makeSpatialScaler(device: device) else { throw Failure.unsupported }
        self.scaler = scaler
        let source = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm,
            width: 640, height: 480, mipmapped: false)
        source.storageMode = .shared; source.usage = scaler.colorTextureUsage
        let destination = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm,
            width: width, height: height, mipmapped: false)
        destination.storageMode = .private; destination.usage = scaler.outputTextureUsage
        guard let input = device.makeTexture(descriptor: source),
              let output = device.makeTexture(descriptor: destination) else { throw Failure.allocation }
        self.input = input; self.output = output
        scaler.inputContentWidth = 640; scaler.inputContentHeight = 480
        scaler.colorTexture = input; scaler.outputTexture = output
    }

    func encode(rgba: UnsafeRawPointer, into command: MTLCommandBuffer) {
        input.replace(region: MTLRegionMake2D(0, 0, 640, 480), mipmapLevel: 0,
                      withBytes: rgba, bytesPerRow: 2560)
        scaler.encode(commandBuffer: command)
    }
}

#if MELEE_GAME_RUNTIME
import SwiftUI
import MetalKit

struct MetalFXFrame: View {
    let frame: CGImage
    let scale: Int
    let failed: (String) -> Void
    var body: some View {
        MetalFXSurface(frame: frame, scale: scale, failed: failed)
            .aspectRatio(4.0 / 3.0, contentMode: .fit)
            .allowsHitTesting(false)
    }
}

private final class MetalFXPresenter: NSObject, MTKViewDelegate {
    let view: MTKView
    private let renderer: MeleeMetalFXRenderer
    private let queue: MTLCommandQueue
    private let available = DispatchSemaphore(value: 1)
    private let diagnostics = ProcessInfo.processInfo.environment["MELEE_DEVICE_DIAGNOSTICS"] == "1"
    private var diagnosticTime: TimeInterval = 0
    private var diagnosticSequence = 0
    private var missingDrawableSince: TimeInterval?
    private var pendingImage: CGImage?
    private let failed: (String) -> Void
    init(scale: Int, failed: @escaping (String) -> Void) throws {
        guard let device = MTLCreateSystemDefaultDevice(), let queue = device.makeCommandQueue() else {
            throw MeleeMetalFXRenderer.Failure.unsupported
        }
        self.queue = queue; self.failed = failed
        renderer = try MeleeMetalFXRenderer(device: device, scale: scale)
        view = MTKView(frame: CGRect(x: 0, y: 0, width: 640, height: 480), device: device)
        view.isPaused = true; view.enableSetNeedsDisplay = false
        view.autoResizeDrawable = false
        view.colorPixelFormat = .bgra8Unorm; view.framebufferOnly = false
        // The VI image is opaque; scaler alpha is not window transparency.
        #if os(macOS)
        view.layer?.isOpaque = true
        #else
        view.isOpaque = true; view.layer.isOpaque = true
        #endif
        view.drawableSize = CGSize(width: renderer.width, height: renderer.height)
        super.init()
        view.delegate = self
    }
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
    func show(_ image: CGImage) {
        pendingImage = image
        guard view.window != nil, view.bounds.width > 0, view.bounds.height > 0 else { return }
        view.draw()
    }
    func draw(in view: MTKView) {
        guard let image = pendingImage else { return }
        // SwiftUI calls update before attaching and laying out a representable.
        // Asking CAMetalLayer for a drawable in that state can fail on iOS.
        guard view.window != nil, view.bounds.width > 0, view.bounds.height > 0 else { return }
        guard image.width == 640, image.height == 480, image.bytesPerRow == 2560,
              let data = image.dataProvider?.data, CFDataGetLength(data) >= 640 * 480 * 4,
              let bytes = CFDataGetBytePtr(data), available.wait(timeout: .now()) == .success else { return }
        guard let drawable = view.currentDrawable, let command = queue.makeCommandBuffer() else {
            available.signal()
            // A view may not have a drawable while being attached. Persistent
            // allocation failure must return to the original framebuffer.
            let now = ProcessInfo.processInfo.systemUptime
            if let since = missingDrawableSince, now - since >= 1 {
                if diagnostics { NSLog("Melee MetalFX display allocation failed after layout: bounds=%@ drawable=%@", String(describing: view.bounds.size), String(describing: view.drawableSize)) }
                DispatchQueue.main.async { self.failed("MetalFX could not allocate a display buffer.") }
            } else if missingDrawableSince == nil {
                missingDrawableSince = now
                if diagnostics { NSLog("Melee MetalFX drawable unavailable: bounds=%@ drawable=%@", String(describing: view.bounds.size), String(describing: view.drawableSize)) }
            }
            return
        }
        missingDrawableSince = nil
        renderer.encode(rgba: bytes, into: command)
        guard let blit = command.makeBlitCommandEncoder() else {
            available.signal()
            DispatchQueue.main.async { self.failed("MetalFX could not present this image.") }; return
        }
        blit.copy(from: renderer.output, sourceSlice: 0, sourceLevel: 0,
            sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
            sourceSize: MTLSize(width: renderer.width, height: renderer.height, depth: 1),
            to: drawable.texture, destinationSlice: 0, destinationLevel: 0,
            destinationOrigin: MTLOrigin(x: 0, y: 0, z: 0))
        var capture: MTLBuffer?
        let now = ProcessInfo.processInfo.systemUptime
        if diagnostics && now - diagnosticTime >= 2 {
            diagnosticTime = now; diagnosticSequence += 1
            capture = renderer.device.makeBuffer(length: renderer.width * renderer.height * 4, options: .storageModeShared)
            if let capture {
                blit.copy(from: renderer.output, sourceSlice: 0, sourceLevel: 0,
                    sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
                    sourceSize: MTLSize(width: renderer.width, height: renderer.height, depth: 1),
                    to: capture, destinationOffset: 0, destinationBytesPerRow: renderer.width * 4,
                    destinationBytesPerImage: renderer.width * renderer.height * 4)
            }
        }
        let capturedBuffer = capture, capturedSequence = diagnosticSequence
        blit.endEncoding(); command.present(drawable)
        command.addCompletedHandler { [self] buffer in
            available.signal()
            if buffer.status == .completed, let capturedBuffer {
                let width = renderer.width, height = renderer.height
                let pixels = Data(bytes: capturedBuffer.contents(), count: width * height * 4)
                DispatchQueue.global(qos: .utility).async {
                    Self.recordCapture(pixels, width: width, height: height, sequence: capturedSequence, uptime: now)
                }
            }
            if buffer.status == .error {
                let message = buffer.error?.localizedDescription ?? "MetalFX rendering failed."
                DispatchQueue.main.async { self.failed(message) }
            }
        }
        command.commit()
        // MTKView owns drawable acquisition/release around its draw callback.
    }
    private static func recordCapture(_ pixels: Data, width: Int, height: Int, sequence: Int, uptime: TimeInterval) {
        guard let provider = CGDataProvider(data: pixels as CFData),
              let image = CGImage(width: width, height: height, bitsPerComponent: 8, bitsPerPixel: 32,
                bytesPerRow: width * 4, space: CGColorSpaceCreateDeviceRGB(),
                bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedFirst.rawValue | CGBitmapInfo.byteOrder32Little.rawValue),
                provider: provider, decode: nil, shouldInterpolate: false, intent: .defaultIntent) else { return }
        do {
            let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
                .appendingPathComponent("Diagnostics", isDirectory: true)
            try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
            let encoded = NSMutableData()
            guard let destination = CGImageDestinationCreateWithData(encoded, UTType.png.identifier as CFString, 1, nil) else { return }
            CGImageDestinationAddImage(destination, image, nil)
            guard CGImageDestinationFinalize(destination) else { return }
            try (encoded as Data).write(to: directory.appendingPathComponent("metalfx-frame.png"), options: .atomic)
            let state: [String: Any] = ["width": width, "height": height, "sequence": sequence, "uptime": uptime]
            try JSONSerialization.data(withJSONObject: state, options: [.sortedKeys])
                .write(to: directory.appendingPathComponent("metalfx.json"), options: .atomic)
        } catch { NSLog("Melee MetalFX diagnostics: %@", error.localizedDescription) }
    }

}

private final class MetalFXCoordinator {
    var presenter: MetalFXPresenter?
    func make(scale: Int, failed: @escaping (String) -> Void) -> MTKView {
        do {
            let presenter = try MetalFXPresenter(scale: scale, failed: failed)
            self.presenter = presenter; return presenter.view
        } catch {
            DispatchQueue.main.async { failed(error.localizedDescription) }
            return MTKView()
        }
    }
}
#if os(macOS)
private struct MetalFXSurface: NSViewRepresentable {
    let frame: CGImage; let scale: Int; let failed: (String) -> Void
    func makeCoordinator() -> MetalFXCoordinator { MetalFXCoordinator() }
    func makeNSView(context: Context) -> MTKView { context.coordinator.make(scale: scale, failed: failed) }
    func updateNSView(_ view: MTKView, context: Context) { context.coordinator.presenter?.show(frame) }
}
#else
private struct MetalFXSurface: UIViewRepresentable {
    let frame: CGImage; let scale: Int; let failed: (String) -> Void
    func makeCoordinator() -> MetalFXCoordinator { MetalFXCoordinator() }
    func makeUIView(context: Context) -> MTKView { context.coordinator.make(scale: scale, failed: failed) }
    func updateUIView(_ view: MTKView, context: Context) { context.coordinator.presenter?.show(frame) }
}
#endif
#endif

#else
// Apple does not ship MetalFX in the iOS Simulator SDK.
enum MeleeMetalFXRenderer { static let supported = false }
#if MELEE_GAME_RUNTIME
import SwiftUI
struct MetalFXFrame: View {
    let frame: CGImage; let scale: Int; let failed: (String) -> Void
    var body: some View {
        Image(decorative: frame, scale: 1).resizable().interpolation(.none).scaledToFit()
    }
}
#endif
#endif
