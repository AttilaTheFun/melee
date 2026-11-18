#if MELEE_GAME_RUNTIME
import SwiftUI
import Combine
import GameController
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

/// File I/O and live stack sampling stay off the display/main thread. The queue
/// serializes cancellation, deletion, and capture, so disabling really stops it.
@MainActor private final class HitchProfiler: ObservableObject {
    @Published private(set) var status = "Off"
    private let queue = DispatchQueue(label: "melee.hitch-profiler", qos: .utility)
    private var timer: DispatchSourceTimer?
    private var enabled = false
    private var active = false
    private var lastError: String?
    static var logURL: URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Diagnostics/profile.jsonl")
    }
    static var previousURL: URL { logURL.deletingLastPathComponent().appendingPathComponent("profile-previous.jsonl") }
    var logs: [URL] { [Self.logURL, Self.previousURL].filter { FileManager.default.fileExists(atPath: $0.path) } }
    func configure(enabled: Bool, active: Bool) {
        guard enabled != self.enabled || active != self.active else { return }
        if enabled && !self.enabled { lastError = nil }
        self.enabled = enabled; self.active = active
        timer?.cancel(); timer = nil
        status = enabled ? (lastError ?? (active ? "Recording locally" : "Paused")) : "Off"
        guard enabled && active && lastError == nil else { return }
        let log = Self.logURL, previous = Self.previousURL
        let source = DispatchSource.makeTimerSource(queue: queue)
        var lastSummary = 0.0
        var captureCount = 0
        var buffer = [CChar](repeating: 0, count: 32_768)
        var failed = false
        func append(_ data: Data) throws {
            let manager = FileManager.default
            try manager.createDirectory(at: log.deletingLastPathComponent(), withIntermediateDirectories: true)
            let size = (try? manager.attributesOfItem(atPath: log.path)[.size] as? NSNumber)?.intValue ?? 0
            if size + data.count > 5_000_000 {
                if manager.fileExists(atPath: previous.path) { try manager.removeItem(at: previous) }
                try manager.moveItem(at: log, to: previous)
            }
            if !manager.fileExists(atPath: log.path) { manager.createFile(atPath: log.path, contents: nil) }
            let file = try FileHandle(forWritingTo: log)
            defer { try? file.close() }
            try file.seekToEnd(); try file.write(contentsOf: data); try file.write(contentsOf: Data([10]))
        }
        source.setEventHandler { [weak self] in
            guard !failed else { return }
            do {
                let count = melee_runtime_capture_hitch(&buffer, buffer.count)
                if count > 0 {
                    try buffer.withUnsafeBytes { try append(Data($0.prefix(Int(count)))) }
                    captureCount += 1
                }
                let now = ProcessInfo.processInfo.systemUptime
                if now - lastSummary >= 2 {
                    lastSummary = now
                    var timing = MeleeRuntimeTiming()
                    melee_runtime_take_timing(&timing)
                    let values: [String: Any] = [
                        "type": "timing", "timestamp": Date().timeIntervalSince1970, "uptime": now,
                        "version": Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "unknown",
                        "os": ProcessInfo.processInfo.operatingSystemVersionString,
                        "thermalState": ProcessInfo.processInfo.thermalState.rawValue,
                        "metalfxScale": UserDefaults.standard.integer(forKey: "metalfxScale"),
                        "widescreen": UserDefaults.standard.bool(forKey: "widescreen"),
                        "frames": timing.frames, "maxIntervalMs": Double(timing.max_interval_ns)/1e6,
                        "over20ms": timing.over20ms, "over33ms": timing.over33ms,
                        "pipelineWaits": timing.pipeline_waits, "pipelineWaitMs": Double(timing.pipeline_wait_ns)/1e6,
                        "pipelineBuilds": timing.pipeline_builds, "pipelineBuildMs": Double(timing.pipeline_build_ns)/1e6,
                        "maxSubmitMs": Double(timing.max_submit_ns)/1e6,
                        "maxGPUWaitMs": Double(timing.max_wait_ns)/1e6,
                        "maxPresentationMs": Double(timing.max_readback_ns)/1e6,
                        "directPresents": timing.direct_presents, "cpuReadbacks": timing.cpu_readbacks]
                    try append(JSONSerialization.data(withJSONObject: values, options: [.sortedKeys]))
                    let text = "Recording locally · \(captureCount) hitch captures this session"
                    Task { @MainActor [weak self] in
                        if self?.enabled == true && self?.active == true { self?.status = text }
                    }
                }
            } catch {
                failed = true
                let text = "Could not record: \(error.localizedDescription)"
                Task { @MainActor [weak self] in
                    self?.lastError = text
                    if self?.enabled == true { self?.status = text }
                }
            }
        }
        // A 10ms check finds the still-stalled thread; it does not wait for a
        // frame to recover. At most one suspension/second, enforced natively.
        source.schedule(deadline: .now(), repeating: .milliseconds(10), leeway: .milliseconds(2))
        timer = source; source.resume()
    }
    func clear() {
        let wasEnabled = enabled, wasActive = active
        configure(enabled: false, active: false)
        let urls = [Self.logURL, Self.previousURL]
        queue.async {
            for url in urls { try? FileManager.default.removeItem(at: url) }
        }
        configure(enabled: wasEnabled, active: wasActive)
    }
    deinit { timer?.cancel() }
}

/// Low-rate local session history. No screen captures, input history, disc paths,
/// or device identifiers. Disk work is serialized and bounded independently of play.
@MainActor private final class SessionDiagnostics {
    static let shared = SessionDiagnostics()
    private let queue = DispatchQueue(label: "melee.session-log", qos: .utility)
    private let slots = DispatchSemaphore(value: 8)
    private let session = UUID().uuidString
    private var lastSample = 0.0
    private var lastFrame: UInt64 = 0
    private var previousContext: UInt64 = .max
    private var previousState: Int32 = -1
    private(set) var error = ""
    static var urls: [URL] {
        let root = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Diagnostics")
        return ["session.jsonl", "session-previous.jsonl"].map { root.appendingPathComponent($0) }
    }
    static var logs: [URL] { urls.filter { FileManager.default.fileExists(atPath: $0.path) } }
    private init() { record("session_start") }
    func record(_ event: String, details: [String: Any] = [:]) {
        var values = details
        values["event"] = event
        values["schema"] = 1
        values["session"] = session
        values["timestamp"] = Date().timeIntervalSince1970
        values["uptimeNs"] = UInt64(ProcessInfo.processInfo.systemUptime * 1e9)
        values["build"] = Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "unknown"
        values["os"] = ProcessInfo.processInfo.operatingSystemVersionString
        values["thermalState"] = ProcessInfo.processInfo.thermalState.rawValue
        values["lowPower"] = ProcessInfo.processInfo.isLowPowerModeEnabled
        values["metalfxScale"] = UserDefaults.standard.integer(forKey: "metalfxScale")
        values["widescreen"] = UserDefaults.standard.bool(forKey: "widescreen")
        values["metalState"] = melee_runtime_metal_state()
        values["frame"] = melee_runtime_frame_sequence()
        values["runtimeState"] = melee_runtime_state()
        values["controllerCategories"] = GCController.controllers().map { $0.productCategory }
        let context = melee_runtime_session_context()
        if context != UInt64.max {
            values["mode"] = (context >> 32) & 65535
            values["scene"] = (context >> 16) & 65535
            values["stage"] = context & 65535
        }
        guard let data = try? JSONSerialization.data(withJSONObject: values, options: [.sortedKeys]) else { return }
        guard slots.wait(timeout: .now()) == .success else { error = "Session log queue full"; return }
        let urls = Self.urls, slots = self.slots
        queue.async {
            defer { slots.signal() }
            do {
                let manager = FileManager.default, current = urls[0], previous = urls[1]
                try manager.createDirectory(at: current.deletingLastPathComponent(), withIntermediateDirectories: true)
                let size = (try? manager.attributesOfItem(atPath: current.path)[.size] as? NSNumber)?.intValue ?? 0
                if size + data.count > 2_000_000 {
                    if manager.fileExists(atPath: previous.path) { try manager.removeItem(at: previous) }
                    try manager.moveItem(at: current, to: previous)
                }
                if !manager.fileExists(atPath: current.path) { manager.createFile(atPath: current.path, contents: nil) }
                let file = try FileHandle(forWritingTo: current)
                defer { try? file.close() }
                try file.seekToEnd(); try file.write(contentsOf: data); try file.write(contentsOf: Data([10]))
            } catch {
                Task { @MainActor in self.error = "Could not write session log" }
            }
        }
    }
    func sample(audioError: String) {
        let now = ProcessInfo.processInfo.systemUptime
        let context = melee_runtime_session_context(), state = melee_runtime_state()
        if context != previousContext || state != previousState {
            previousContext = context; previousState = state
            var details: [String: Any] = [:]
            if state == MELEE_RUNTIME_FAILED {
                var buffer = [CChar](repeating: 0, count: 1024)
                melee_runtime_error(&buffer, buffer.count)
                details["runtimeError"] = String(cString: buffer)
            }
            record("game_transition", details: details)
        }
        guard now - lastSample >= 2 else { return }
        let frame = melee_runtime_frame_sequence()
        var shaders = MeleeShaderStatus(); melee_runtime_shader_status(&shaders)
        var timing = MeleeRuntimeTiming(); melee_runtime_session_timing(&timing)
        record("session_sample", details: [
            "totalFrames": timing.frames, "totalOver20ms": timing.over20ms, "totalOver33ms": timing.over33ms,
            "maxIntervalMs": Double(timing.max_interval_ns) / 1e6,
            "maxSubmitMs": Double(timing.max_submit_ns) / 1e6,
            "maxGPUWaitMs": Double(timing.max_wait_ns) / 1e6,
            "maxPresentationMs": Double(timing.max_readback_ns) / 1e6,
            "sampleSeconds": lastSample == 0 ? 0 : now - lastSample,
            "completedFrames": frame >= lastFrame ? frame - lastFrame : 0,
            "cpuReadbacks": melee_runtime_cpu_readbacks(),
            "audioError": audioError, "knownShaders": shaders.known,
            "pendingShaders": shaders.remaining, "shaderDiscoveries": shaders.discoveries,
            "shaderLogFailed": shaders.log_failed != 0, "catalogFailed": shaders.catalog_failed != 0,
            "shaderEventsDropped": shaders.dropped_events])
        lastSample = now; lastFrame = frame
    }
}

@MainActor final class GameRuntimeModel: ObservableObject {
    @Published var started = false
    @Published var frame: CGImage?
    @Published var message = "Choose your Melee disc image to start."
    @Published var audioError = ""
    @Published var shaderSummary = "Known shaders are prepared before each game session."
    static var shaderLogs: [URL] {
        guard let cache = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first else { return [] }
        return ["shader-events.jsonl", "shader-events-previous.jsonl"].map {
            cache.appendingPathComponent("MeleeNative/GPU").appendingPathComponent($0)
        }.filter { FileManager.default.fileExists(atPath: $0.path) }
    }
    private var shaderStatusTime: TimeInterval = 0
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
        SessionDiagnostics.shared.record(value ? "resume" : "pause")
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
            // The immutable bundled catalog augments, never replaces, the
            // player's learned pipeline_cache.db (including its live WAL).
            if let seed = Bundle.main.url(forResource: "initial_pipeline_cache", withExtension: "db") {
                try Data(contentsOf: seed).write(to: cache.appendingPathComponent("initial_pipeline_cache.db"), options: .atomic)
            }
            var saves = try FileManager.default.url(for: .applicationSupportDirectory, in: .userDomainMask,
                appropriateFor: nil, create: true).appendingPathComponent("MeleeNative", isDirectory: true)
            #if targetEnvironment(simulator)
            if let agent = ProcessInfo.processInfo.environment["MELEE_AGENT_DIR"] {
                saves = URL(fileURLWithPath: agent).appendingPathComponent("saves")
            }
            #endif
            try FileManager.default.createDirectory(at: saves, withIntermediateDirectories: true)
            let cardPath = saves.appendingPathComponent("MemoryCardA.card").path
            disc = url; scoped = url.startAccessingSecurityScopedResource()
            SessionDiagnostics.shared.record("game_start")
            started = true; message = "Starting Melee…"
            let path = url.path, cachePath = cache.path
            Thread { _ = melee_runtime_run_with_save(path, cachePath, cardPath) }.start()
        } catch { message = error.localizedDescription }
    }

    func sample() {
        SessionDiagnostics.shared.sample(audioError: audioError)
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
        let now = ProcessInfo.processInfo.systemUptime
        if now - shaderStatusTime >= 0.25 {
            shaderStatusTime = now
            var shaders = MeleeShaderStatus()
            melee_runtime_shader_status(&shaders)
            if shaders.preparing != 0 {
                message = shaders.known == 0 ? "Checking known shaders…" :
                    "Preparing shaders… \(shaders.known - min(shaders.known, shaders.remaining)) / \(shaders.known)"
            } else if melee_runtime_state() == MELEE_RUNTIME_STARTING {
                message = "Starting Melee…"
            }
            let summary = "\(shaders.known) known shaders · \(shaders.discoveries) discovered this session" +
                (shaders.log_failed != 0 ? " · Could not write shader log" : "") +
                (shaders.catalog_failed != 0 ? " · Shader catalog unavailable; learning will not persist" : "") +
                (shaders.dropped_events > 0 ? " · \(shaders.dropped_events) log events dropped" : "")
            if shaderSummary != summary { shaderSummary = summary }
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
        guard deviceDiagnostics, !UserDefaults.standard.bool(forKey: "hitchProfiling") else { return }
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
                "pipelineWaits": timing.pipeline_waits, "pipelineWaitMs": Double(timing.pipeline_wait_ns) / 1e6,
                "pipelineBuilds": timing.pipeline_builds, "pipelineBuildMs": Double(timing.pipeline_build_ns) / 1e6,
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
                "controllers": GCController.controllers().filter { $0.extendedGamepad != nil }.map { $0.vendorName ?? "Game controller" },
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
    @AppStorage("widescreen") private var widescreen = false
    @AppStorage("hitchProfiling") private var hitchProfiling = false
    @StateObject private var profiler = HitchProfiler()
    private var displayAspect: CGFloat { widescreen ? 16.0 / 9.0 : 4.0 / 3.0 }
    @State private var upscaleError = ""
    @StateObject private var frameClock = GameFrameClock()
    private func updateProfiling() {
        profiler.configure(enabled: hitchProfiling, active: phase == .active && !graphics && !bindings)
    }
    private var gameCanvas: some View {
        ZStack {
            Color.black
            if upscaleScale == 0 && !game.directFailed &&
                ProcessInfo.processInfo.environment["MELEE_CPU_PRESENTATION"] != "1" {
                DirectMetalSurface().aspectRatio(displayAspect, contentMode: .fit).allowsHitTesting(false)
            } else if let frame = game.frame {
                if (2...3).contains(upscaleScale), MeleeMetalFXRenderer.supported, upscaleError.isEmpty {
                    MetalFXFrame(frame: frame, scale: upscaleScale, aspect: displayAspect) { upscaleError = $0 }.id(upscaleScale)
                } else {
                    Image(decorative: frame, scale: 1).resizable().interpolation(.none).aspectRatio(displayAspect, contentMode: .fit)
                }
            }
            if !game.message.isEmpty {
                Text(game.message).foregroundStyle(.white).padding().background(.black.opacity(0.8))
            }
        }
    }
    private func openSettings() { input.clear(); graphics = true; game.setActive(false) }
    @ViewBuilder private var display: some View {
        #if os(iOS)
        GeometryReader { geometry in
            let height = min(geometry.size.height, geometry.size.width / displayAspect)
            let width = height * displayAspect
            ZStack {
                Color.black
                gameCanvas.frame(width: width, height: height)
                if !input.hasPlayerOneController { TouchSurface(input: input, gameOverlay: true) }
                if ProcessInfo.processInfo.environment["MELEE_UI_FRAME_CLOCK_TEST"] == "1" {
                    VStack {
                        Text(String(game.frameSequence)).accessibilityIdentifier("melee.test.frameSequence")
                        Text(String(melee_runtime_metal_state())).accessibilityIdentifier("melee.test.metalState")
                        Text(String(melee_runtime_cpu_readbacks())).accessibilityIdentifier("melee.test.readbacks")
                        Text(widescreen ? "16:9" : "4:3").accessibilityIdentifier("melee.test.aspect")
                    }.position(x: 40, y: 80)
                }
                Button(action: openSettings) {
                    Image(systemName: "gearshape.fill").font(.title2)
                        .frame(width: 48, height: 48).background(.black.opacity(0.55), in: Circle())
                }
                .accessibilityLabel("Settings").accessibilityIdentifier("melee.settings")
                .position(x: geometry.size.width - max(32, (geometry.size.width - width) / 4),
                          y: widescreen && !input.hasPlayerOneController ? geometry.size.height * 0.26 : 40)
            }.frame(width: geometry.size.width, height: geometry.size.height)
        }
        .ignoresSafeArea().statusBarHidden().persistentSystemOverlays(.hidden)
        .preferredColorScheme(.dark)
        #else
        VStack(spacing: 8) {
            HStack {
                Text("Melee").font(.headline)
                if ProcessInfo.processInfo.environment["MELEE_UI_FRAME_CLOCK_TEST"] == "1" {
                    Text(String(game.frameSequence)).accessibilityIdentifier("melee.test.frameSequence")
                    Text(String(melee_runtime_metal_state())).accessibilityIdentifier("melee.test.metalState")
                    Text(String(melee_runtime_cpu_readbacks())).accessibilityIdentifier("melee.test.readbacks")
                        Text(widescreen ? "16:9" : "4:3").accessibilityIdentifier("melee.test.aspect")
                }
                Spacer()
                if !game.started { Button("Open disc image…") { importing = true } }
                Button("Graphics…") { input.clear(); graphics = true; game.setActive(false) }
                #if os(macOS)
                Button("Controls…") { input.clear(); bindings = true; game.setActive(false) }
                #endif
            }.padding(.horizontal)
            gameCanvas.aspectRatio(displayAspect, contentMode: .fit)
            Text("Move: \(input.keyLabel(0)) \(input.keyLabel(1)) \(input.keyLabel(2)) \(input.keyLabel(3)) · Attack: \(input.keyLabel(8)) · Special: \(input.keyLabel(9)) · Jump: \(input.keyLabel(10))/\(input.keyLabel(11)) · Shield: \(input.keyLabel(13))/\(input.keyLabel(14)) · Grab: \(input.keyLabel(12)) · Start: \(input.keyLabel(15))")
                .multilineTextAlignment(.center).fixedSize(horizontal: false, vertical: true)
                .font(.caption).foregroundStyle(.secondary)
            if !input.controllerNames.isEmpty {
                Text(input.controllerNames.joined(separator: " · ")).font(.caption)
            }
            if !game.audioError.isEmpty {
                Text("Audio: \(game.audioError)").font(.caption).foregroundStyle(.red)
            }
        }
        .padding(.vertical, 8)
        #endif
    }
    var body: some View {
        display
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
            NavigationStack {
                Form {
                    Section("Debugging") {
                        if !(GameRuntimeModel.shaderLogs + SessionDiagnostics.logs).isEmpty {
                            ShareLink(items: GameRuntimeModel.shaderLogs + SessionDiagnostics.logs + profiler.logs) { Label("Share debugging logs", systemImage: "square.and.arrow.up") }
                            Button("Mark a visual problem") { SessionDiagnostics.shared.record("user_visual_problem") }
                            Text("Includes session history, graphics settings, controller categories, thermal state, shader events and any enabled hitch profiles. Session history stays local; up to 4 MB is kept. Mark a problem after noticing it to help locate the scene.").font(.caption).foregroundStyle(.secondary)
                            if !SessionDiagnostics.shared.error.isEmpty { Text(SessionDiagnostics.shared.error) }
                        }
                    }
                    Section("Controllers") {
                        if input.controllerNames.isEmpty { Text("No controller connected") }
                        ForEach(input.controllerNames, id: \.self) { Text($0) }
                        Button(input.discoveringControllers ? "Stop searching" : "Pair wireless controller") {
                            if input.discoveringControllers { input.stopControllerDiscovery() }
                            else { input.discoverControllers() }
                        }
                        Text("Put a wireless controller in pairing mode. If it doesn't appear, pair it in iPhone Settings → Bluetooth. Wired controllers connect automatically.")
                            .font(.caption).foregroundStyle(.secondary)
                    }
                    Section {
                        NavigationLink("Controller bindings") {
                            Form {
                        Text("Applies to connected controllers. Left stick moves; right stick controls the C-stick.")
                            .font(.caption).foregroundStyle(.secondary)
                        ForEach(ControllerBindings.sources.indices, id: \.self) { index in
                            Picker(ControllerBindings.sources[index], selection: Binding(
                                get: { input.controllerBindings[index] },
                                set: { input.bindController(index, to: $0) })) {
                                ForEach(ControllerBindings.actions, id: \.1) { action in
                                    Text(action.0).tag(action.1)
                                }
                            }
                        }
                        Button("Reset controller bindings") { input.resetControllerBindings() }
                            }.navigationTitle("Controller bindings")
                        }
                    }
                    Section("Graphics") {
                        Toggle("16:9 widescreen (experimental)", isOn: $widescreen)
                            .accessibilityIdentifier("melee.widescreen")
                        Text("Shows more of 3D scenes; keeps 2D elements centered. Some stage edges and effects may look incomplete.")
                            .font(.caption).foregroundStyle(.secondary)
                        Picker("MetalFX upscaling", selection: $upscaleScale) {
                            Text("Off").tag(0)
                            Text("2×").tag(2)
                            Text("3×").tag(3)
                        }.pickerStyle(.segmented).disabled(!MeleeMetalFXRenderer.supported)
                        Text(MeleeMetalFXRenderer.supported
                            ? "Upscale the original image. Higher resolutions use more GPU power."
                            : "MetalFX is unavailable on this device. The original image will be displayed.")
                            .font(.caption).foregroundStyle(.secondary)
                        if !upscaleError.isEmpty { Text(upscaleError).foregroundStyle(.secondary) }
                    }
                    Section("Shader preparation") {
                        Text(game.shaderSummary).accessibilityIdentifier("melee.shaders.status")
                        Text("New shaders are learned as you play and prepared before the next session. First encounters can still pause briefly. Shader logs stay on this device; up to 4 MB is kept.")
                            .font(.caption).foregroundStyle(.secondary)

                    }
                    Section("Performance profiling") {
                        Toggle("Record performance and hitch stacks", isOn: $hitchProfiling)
                            .accessibilityIdentifier("melee.profiling")
                        Text("Captures the game thread after two missed frame deadlines (about 50 ms without a completed frame), at most once per second. Pauses with the game. Logs stay on this device; up to 10 MB is kept. Sampling adds a small amount of overhead.")
                            .font(.caption).foregroundStyle(.secondary)
                        Text(profiler.status).font(.caption).accessibilityIdentifier("melee.profiling.status")
                        if !profiler.logs.isEmpty {
                            ShareLink(items: profiler.logs) { Label("Share profiling logs", systemImage: "square.and.arrow.up") }
                            Button("Clear profiling logs") { profiler.clear() }
                        }
                    }
                    if !game.audioError.isEmpty { Section("Audio") { Text(game.audioError) } }
                    if !game.started { Button("Open disc image…") { graphics = false; importing = true } }
                }
                .navigationTitle("Settings")
                .toolbar { ToolbarItem(placement: .confirmationAction) { Button("Done") { graphics = false } } }
            }
            .onDisappear { input.stopControllerDiscovery() }
            #if os(macOS)
            .frame(minWidth: 520, minHeight: 550)
            #endif
        }
        .onChange(of: hitchProfiling) { _ in updateProfiling() }
        .onChange(of: graphics) { _ in updateProfiling() }
        .onChange(of: bindings) { _ in updateProfiling() }
        .onChange(of: widescreen) { melee_runtime_set_widescreen($0 ? 1 : 0); SessionDiagnostics.shared.record("graphics_changed") }
        .onChange(of: upscaleScale) { _ in upscaleError = ""; SessionDiagnostics.shared.record("graphics_changed") }
        .onChange(of: upscaleError) { value in SessionDiagnostics.shared.record("upscaler_status", details: ["error": value]) }
        .onChange(of: input.controllerNames) { _ in SessionDiagnostics.shared.record("controllers_changed") }
        .onAppear { if phase == .active { frameClock.start() } }
        .onDisappear { frameClock.stop(); profiler.configure(enabled: false, active: false) }
        .onReceive(frameClock.ticks) { _ in
            #if targetEnvironment(simulator)
            if ProcessInfo.processInfo.environment["MELEE_AGENT_DIR"] == nil {
                input.sample(deliver: !bindings && !graphics)
            }
            #else
            input.sample(deliver: !bindings && !graphics)
            #endif
            game.sample()
        }
        .onChange(of: phase) { value in
            SessionDiagnostics.shared.record("lifecycle", details: ["phase": String(describing: value)])
            if value == .active { frameClock.start() } else { frameClock.stop() }
            input.active = value == .active
            if !input.active { input.clear() }
            game.setActive(input.active && !bindings && !graphics)
            updateProfiling()
        }
        .fileImporter(isPresented: $importing, allowedContentTypes: [.data]) { result in
            if case .success(let url) = result { input.clear(); game.start(url) }
        }
        .onAppear {
            melee_runtime_set_widescreen(widescreen ? 1 : 0)
            game.setActive(phase == .active)
            updateProfiling()
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
