#if MELEE_GAME_RUNTIME
import SwiftUI
import Combine
import UniformTypeIdentifiers
import ImageIO
#if os(macOS)
import AppKit
#endif

/// Keep the clock's lifetime independent of SwiftUI view reconstruction.
/// iOS polls just before display refresh instead of on a drifting run-loop timer.
@MainActor private final class GameFrameClock: NSObject, ObservableObject {
    let ticks = PassthroughSubject<Void, Never>()
    #if os(iOS)
    @MainActor private final class Target: NSObject {
        weak var owner: GameFrameClock?
        @objc func tick() {
            guard let owner else { return }
            owner.ticks.send()
            // Publish input and consume the completed frame before waking the
            // next game retrace. Game work stays off the main/display thread.
            melee_runtime_display_tick()
        }
    }
    private var link: CADisplayLink?
    #else
    private var timer: Timer?
    #endif
    func start() {
        #if os(iOS)
        guard link == nil else { return }
        melee_runtime_set_display_clock(1)
        let target = Target(); target.owner = self
        let link = CADisplayLink(target: target, selector: #selector(Target.tick))
        link.preferredFrameRateRange = CAFrameRateRange(minimum: 60, maximum: 60, preferred: 60)
        link.add(to: .main, forMode: .common)
        self.link = link
        #else
        guard timer == nil else { return }
        timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 60, repeats: true) { [weak self] _ in
            self?.ticks.send()
        }
        RunLoop.main.add(timer!, forMode: .common)
        #endif
    }
    func stop() {
        #if os(iOS)
        link?.invalidate(); link = nil
        melee_runtime_set_display_clock(0)
        #else
        timer?.invalidate(); timer = nil
        #endif
    }
    deinit {
        #if os(iOS)
        link?.invalidate()
        melee_runtime_set_display_clock(0)
        #else
        timer?.invalidate()
        #endif
    }
}

@MainActor final class GameRuntimeModel: ObservableObject {
    @Published var started = false
    @Published var frame: CGImage?
    @Published var message = "Choose your Melee disc image to start."
    @Published var audioError = ""
    private var audio: MeleeAudioOutput?
    // UI automation can exercise rendering/input without opening an audio device.
    private let audioEnabled = ProcessInfo.processInfo.environment["MELEE_TEST_SILENT"] != "1"
    private var active = false
    @Published private var sequence: UInt64 = 0
    @Published var directFailed = false
    var frameSequence: UInt64 { sequence }
    private let pixels = UnsafeMutableRawPointer.allocate(byteCount: 640 * 480 * 4, alignment: 64)
    private var disc: URL?
    private var scoped = false
    private let deviceDiagnostics = ProcessInfo.processInfo.environment["MELEE_DEVICE_DIAGNOSTICS"] == "1"
    private let performanceOnly = ProcessInfo.processInfo.environment["MELEE_PERF_ONLY"] == "1"
    private var sampleTicks = 0, presentedFrames = 0, skippedFrames = 0, lateTicks = 0
    private var previousTick: TimeInterval = 0, maximumTick: TimeInterval = 0
    private var imageTime: TimeInterval = 0
    private var diagnosticTime: TimeInterval = 0
    private var diagnosticSequence: UInt64 = 0
    deinit { pixels.deallocate() }

    func setActive(_ value: Bool) {
        active = value
        if !value { audio?.stop(); audio = nil }
        else { audioError = "" }
        melee_runtime_set_paused(value ? 0 : 1)
    }

    private func updateAudio() {
        guard audioEnabled, active, sequence > 0, audioError.isEmpty else { return }
        if let audio {
            if audio.mixerStatus == MELEE_AUDIO_FAILED || audio.recoveryError != nil {
                audioError = audio.recoveryError?.localizedDescription ?? "The game audio mixer stopped."
                audio.stop(); self.audio = nil
            }
            return
        }
        do {
            let output = try MeleeAudioOutput()
            try output.startMixer()
            try output.start()
            audio = output
        } catch { audioError = error.localizedDescription }
    }

    func start(_ url: URL) {
        guard !started else { return }
        do {
            let cache = try FileManager.default.url(for: .cachesDirectory, in: .userDomainMask,
                appropriateFor: nil, create: true).appendingPathComponent("MeleeNative/GPU", isDirectory: true)
            try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
            let saves = try FileManager.default.url(for: .applicationSupportDirectory, in: .userDomainMask,
                appropriateFor: nil, create: true).appendingPathComponent("MeleeNative", isDirectory: true)
            try FileManager.default.createDirectory(at: saves, withIntermediateDirectories: true)
            let cardPath = saves.appendingPathComponent("MemoryCardA.card").path
            disc = url; scoped = url.startAccessingSecurityScopedResource()
            started = true; message = "Starting Melee…"
            let path = url.path, cachePath = cache.path
            Thread { _ = melee_runtime_run_with_save(path, cachePath, cardPath) }.start()
        } catch { message = error.localizedDescription }
    }

    func sample() {
        if deviceDiagnostics {
            let now = ProcessInfo.processInfo.systemUptime
            if previousTick > 0 {
                let gap = now - previousTick
                maximumTick = max(maximumTick, gap)
                if gap > 0.025 { lateTicks += 1 }
            }
            previousTick = now; sampleTicks += 1
        }
        if melee_runtime_state() == MELEE_RUNTIME_FAILED {
            audio?.stop(); audio = nil
            var text = [CChar](repeating: 0, count: 1024)
            melee_runtime_error(&text, text.count); message = String(cString: text)
            return
        }
        updateAudio()
        let direct = melee_runtime_metal_state()
        if direct < 0 { directFailed = true }
        let next = direct == 1 ? melee_runtime_frame_sequence() :
            melee_runtime_copy_frame(pixels, 640 * 480 * 4, sequence)
        guard next > sequence else { return }
        if deviceDiagnostics {
            presentedFrames += 1
            if sequence > 0 { skippedFrames += Int(next - sequence - 1) }
        }
        sequence = next
        if direct == 1 {
            message = ""
            recordDeviceDiagnostics()
            return
        }
        let imageStarted = ProcessInfo.processInfo.systemUptime
        let data = Data(bytes: pixels, count: 640 * 480 * 4)
        guard let provider = CGDataProvider(data: data as CFData) else { return }
        frame = CGImage(width: 640, height: 480, bitsPerComponent: 8, bitsPerPixel: 32,
            bytesPerRow: 2560, space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.last.rawValue),
            provider: provider, decode: nil, shouldInterpolate: false, intent: .defaultIntent)
        imageTime += ProcessInfo.processInfo.systemUptime - imageStarted
        message = ""
        recordDeviceDiagnostics()
    }

    // Opt-in device diagnostics live only in this app's container. They let a
    // tethered test collect rendering/audio evidence without a second test app.
    private func recordDeviceDiagnostics() {
        guard deviceDiagnostics else { return }
        let now = ProcessInfo.processInfo.systemUptime
        guard now - diagnosticTime >= 2 else { return }
        let elapsed = now - diagnosticTime
        let fps = diagnosticTime > 0 ? Double(sequence - diagnosticSequence) / elapsed : 0
        diagnosticTime = now; diagnosticSequence = sequence
        do {
            let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
                .appendingPathComponent("Diagnostics", isDirectory: true)
            try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
            var memory = task_vm_info_data_t()
            var memoryCount = mach_msg_type_number_t(MemoryLayout<task_vm_info_data_t>.size / MemoryLayout<integer_t>.size)
            let memoryResult = withUnsafeMutablePointer(to: &memory) { pointer in
                pointer.withMemoryRebound(to: integer_t.self, capacity: Int(memoryCount)) {
                    task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), $0, &memoryCount)
                }
            }
            var timing = MeleeRuntimeTiming()
            melee_runtime_take_timing(&timing)
            let performance: [String: Any] = [
                "nativeFrames": timing.frames, "directPresents": timing.direct_presents, "cpuReadbacks": timing.cpu_readbacks, "intervalMs": Double(timing.interval_ns) / 1e6,
                "maxIntervalMs": Double(timing.max_interval_ns) / 1e6,
                "over20ms": timing.over20ms, "over33ms": timing.over33ms,
                "submitMs": Double(timing.submit_ns) / 1e6, "maxSubmitMs": Double(timing.max_submit_ns) / 1e6,
                "waitMs": Double(timing.wait_ns) / 1e6, "maxWaitMs": Double(timing.max_wait_ns) / 1e6,
                "presentationMs": Double(timing.readback_ns) / 1e6, "maxPresentationMs": Double(timing.max_readback_ns) / 1e6,
                "sampleTicks": sampleTicks, "presentedFrames": presentedFrames, "skippedFrames": skippedFrames,
                "lateTicks": lateTicks, "maxTickMs": maximumTick * 1000, "imageMs": imageTime * 1000,
                "thermalState": ProcessInfo.processInfo.thermalState.rawValue]
            sampleTicks = 0; presentedFrames = 0; skippedFrames = 0; lateTicks = 0
            maximumTick = 0; imageTime = 0
            let values: [String: Any] = ["memoryFootprint": memoryResult == KERN_SUCCESS ? memory.phys_footprint : 0,
                "performance": performance, "sequence": sequence, "framesPerSecond": fps,
                "runtimeState": melee_runtime_state(), "audioRunning": audio?.engine.isRunning ?? false,
                "audioStats": audio?.diagnostics ?? [:], "audioError": audioError, "mixerStatus": audio?.mixerStatus ?? -1,
                "timestamp": Date().timeIntervalSince1970, "uptime": now]
            try JSONSerialization.data(withJSONObject: values, options: [.sortedKeys])
                .write(to: directory.appendingPathComponent("state.json"), options: .atomic)
            if performanceOnly {
                let log = directory.appendingPathComponent("performance.jsonl")
                if !FileManager.default.fileExists(atPath: log.path) {
                    FileManager.default.createFile(atPath: log.path, contents: nil)
                }
                let handle = try FileHandle(forWritingTo: log)
                defer { try? handle.close() }
                let length = try handle.seekToEnd()
                if length < 10_000_000 {
                    try handle.write(contentsOf: JSONSerialization.data(withJSONObject: values, options: [.sortedKeys]))
                    try handle.write(contentsOf: Data([10]))
                }
                return
            }
            let imageData = NSMutableData()
            if let frame, let destination = CGImageDestinationCreateWithData(
                imageData, UTType.png.identifier as CFString, 1, nil) {
                CGImageDestinationAddImage(destination, frame, nil)
                if CGImageDestinationFinalize(destination) {
                    try (imageData as Data).write(to: directory.appendingPathComponent("frame.png"), options: .atomic)
                }
            }
        } catch { NSLog("Melee device diagnostics: %@", error.localizedDescription) }
    }
}

struct GameView: View {
    @ObservedObject var input: InputModel
    @StateObject private var game = GameRuntimeModel()
    @Environment(\.scenePhase) private var phase
    @State private var importing = false
    @State private var bindings = false
    @State private var graphics = false
    @AppStorage("metalfxScale") private var upscaleScale = 0
    @State private var upscaleError = ""
    @StateObject private var frameClock = GameFrameClock()
    private var gameCanvas: some View {
        ZStack {
            Color.black
            if upscaleScale == 0 && !game.directFailed &&
                ProcessInfo.processInfo.environment["MELEE_CPU_PRESENTATION"] != "1" {
                DirectMetalSurface().aspectRatio(4.0 / 3.0, contentMode: .fit).allowsHitTesting(false)
            } else if let frame = game.frame {
                if (2...3).contains(upscaleScale), MeleeMetalFXRenderer.supported, upscaleError.isEmpty {
                    MetalFXFrame(frame: frame, scale: upscaleScale) { upscaleError = $0 }.id(upscaleScale)
                } else {
                    Image(decorative: frame, scale: 1).resizable().interpolation(.none).scaledToFit()
                }
            }
            if !game.message.isEmpty {
                Text(game.message).foregroundStyle(.white).padding().background(.black.opacity(0.8))
            }
        }
    }
    var body: some View {
        VStack(spacing: 8) {
            HStack {
                Text("Melee").font(.headline)
                if ProcessInfo.processInfo.environment["MELEE_UI_FRAME_CLOCK_TEST"] == "1" {
                    Text(String(game.frameSequence)).accessibilityIdentifier("melee.test.frameSequence")
                    Text(String(melee_runtime_metal_state())).accessibilityIdentifier("melee.test.metalState")
                    Text(String(melee_runtime_cpu_readbacks())).accessibilityIdentifier("melee.test.readbacks")
                }
                Spacer()
                if !game.started { Button("Open disc image…") { importing = true } }
                Button("Graphics…") { input.clear(); graphics = true; game.setActive(false) }
                #if os(macOS)
                Button("Controls…") { input.clear(); bindings = true; game.setActive(false) }
                #endif
            }.padding(.horizontal)
            #if os(iOS)
            GeometryReader { geometry in
                ZStack {
                    Color.black
                    gameCanvas.padding(.horizontal, min(150, geometry.size.width * 0.18))
                    TouchSurface(input: input, gameOverlay: true)
                }
            }
            #else
            gameCanvas.aspectRatio(4.0 / 3.0, contentMode: .fit)
            Text("Move: \(input.keyLabel(0)) \(input.keyLabel(1)) \(input.keyLabel(2)) \(input.keyLabel(3)) · Attack: \(input.keyLabel(8)) · Special: \(input.keyLabel(9)) · Jump: \(input.keyLabel(10))/\(input.keyLabel(11)) · Shield: \(input.keyLabel(13))/\(input.keyLabel(14)) · Grab: \(input.keyLabel(12)) · Start: \(input.keyLabel(15))")
                .multilineTextAlignment(.center).fixedSize(horizontal: false, vertical: true)
                .font(.caption).foregroundStyle(.secondary)
            #endif
            if !input.controllerNames.isEmpty {
                Text(input.controllerNames.joined(separator: " · ")).font(.caption)
            }
            if !game.audioError.isEmpty {
                Text("Audio: \(game.audioError)").font(.caption).foregroundStyle(.red)
            }
        }
        .padding(.vertical, 8)
        #if os(macOS)
        .frame(minWidth: 700, minHeight: 570)
        .background(KeyboardSurface(input: input))
        .sheet(isPresented: $bindings, onDismiss: {
            input.clear(); game.setActive(phase == .active && !graphics)
        }) { GameBindingsView(input: input) }
        #endif
        .sheet(isPresented: $graphics, onDismiss: {
            input.clear(); game.setActive(phase == .active && !bindings)
        }) {
            VStack(alignment: .leading, spacing: 16) {
                Text("Graphics").font(.title2)
                Text("MetalFX upscaling").font(.headline)
                Picker("MetalFX upscaling", selection: $upscaleScale) {
                    Text("Off").tag(0)
                    Text("2× (1280 × 960)").tag(2)
                    Text("3× (1920 × 1440)").tag(3)
                }.pickerStyle(.segmented).disabled(!MeleeMetalFXRenderer.supported)
                Text(MeleeMetalFXRenderer.supported
                    ? "Optionally sharpen and enlarge the original game image. Higher resolutions use more GPU power."
                    : "MetalFX is unavailable on this device. The original image will be displayed.")
                    .font(.callout).foregroundStyle(.secondary)
                if !upscaleError.isEmpty {
                    Text("Using the original image: \(upscaleError)").foregroundStyle(.secondary)
                }
                Button("Done") { graphics = false }.keyboardShortcut(.defaultAction)
            }.padding(24).frame(maxWidth: 520)
        }
        .onChange(of: upscaleScale) { _ in upscaleError = "" }
        .onAppear { if phase == .active { frameClock.start() } }
        .onDisappear { frameClock.stop() }
        .onReceive(frameClock.ticks) { _ in
            if !bindings && !graphics { input.sample() }
            game.sample()
        }
        .onChange(of: phase) { value in
            if value == .active { frameClock.start() } else { frameClock.stop() }
            input.active = value == .active
            if !input.active { input.clear() }
            game.setActive(input.active && !bindings && !graphics)
        }
        .fileImporter(isPresented: $importing, allowedContentTypes: [.data]) { result in
            if case .success(let url) = result { input.clear(); game.start(url) }
        }
        .onAppear {
            game.setActive(phase == .active)
            // Supports launching on the private test VM without a file dialog.
            if let path = ProcessInfo.processInfo.environment["MELEE_DISC_IMAGE"] {
                game.start(URL(fileURLWithPath: path))
            } else {
                // A tethered device can receive the user's disc here. Keep it
                // usable when subsequently opened from its home-screen icon.
                let localDisc = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
                    .appendingPathComponent("Melee.ciso")
                if FileManager.default.fileExists(atPath: localDisc.path) { game.start(localDisc) }
            }
        }
    }
}

#if os(macOS)
final class GameAppDelegate: NSObject, NSApplicationDelegate {
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
}

struct GameBindingsView: View {
    @ObservedObject var input: InputModel
    @Environment(\.dismiss) private var dismiss
    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            Text("Keyboard controls").font(.title2)
            Text(input.capture == nil ? "Choose an action to change its key." : "Press a key; Escape cancels.")
            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())]) {
                ForEach(0..<actionNames.count, id: \.self) { action in
                    Button { input.clear(); input.capture = action } label: {
                        HStack { Text(actionNames[action]); Spacer(); Text(input.keyLabel(action)) }
                    }
                }
            }
            HStack {
                Button("Restore defaults") { input.resetBindings() }
                Spacer()
                Button("Done") { input.clear(); dismiss() }
            }
        }.padding(24).frame(width: 640).background(KeyboardSurface(input: input))
    }
}
#endif
#endif
