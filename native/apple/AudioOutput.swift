import AVFAudio

private final class AudioPCMQueue {
    let pointer: OpaquePointer
    init() throws {
        guard let pointer = melee_audio_ring_create() else {
            throw NSError(domain: "MeleeAudio", code: 1,
                          userInfo: [NSLocalizedDescriptionKey: "Unable to allocate the audio queue"])
        }
        self.pointer = pointer
    }
    deinit { melee_audio_ring_destroy(pointer) }
}

/// Own and operate lifecycle methods on the main thread. One separate producer may enqueue PCM while
/// AVAudioEngine consumes it. Game/mixer work must never run in the source block.
final class MeleeAudioOutput {
    let engine: AVAudioEngine
    private let queue: AudioPCMQueue
    private let source: AVAudioSourceNode
    private let offline: Bool
    private var producer: OpaquePointer?
    private var observers: [NSObjectProtocol] = []
    private var wantsPlayback = false
    private var wantsMixer = false
    private var interrupted = false
    private(set) var recoveryError: Error?
    enum Event { case configurationChanged, interruptionBegan, interruptionEnded(Bool), routeLost }

    #if os(iOS)
    private var sessionActivated = false
    #endif

    init(offlineSampleRate: Double? = nil) throws {
        let queue = try AudioPCMQueue()
        let engine = AVAudioEngine()
        if let rate = offlineSampleRate {
            guard rate.isFinite, rate > 0,
                  let format = AVAudioFormat(standardFormatWithSampleRate: rate, channels: 2) else {
                throw NSError(domain: "MeleeAudio", code: 2,
                              userInfo: [NSLocalizedDescriptionKey: "Invalid offline sample rate"])
            }
            try engine.enableManualRenderingMode(.offline, format: format, maximumFrameCount: 4096)
        }
        let pcm = AVAudioFormat(commonFormat: .pcmFormatInt16, sampleRate: 32000,
                                channels: 2, interleaved: true)!
        let source = AVAudioSourceNode(format: pcm) { silence, _, count, list in
            let buffers = UnsafeMutableAudioBufferListPointer(list)
            guard buffers.count == 1, buffers[0].mNumberChannels == 2,
                  count <= MELEE_AUDIO_RING_FRAMES,
                  buffers[0].mDataByteSize >= count * 4,
                  let data = buffers[0].mData else {
                for buffer in buffers {
                    if let data = buffer.mData { memset(data, 0, Int(buffer.mDataByteSize)) }
                }
                silence.pointee = true
                return kAudio_ParamError
            }
            let available = melee_audio_ring_read(queue.pointer,
                data.assumingMemoryBound(to: Int16.self), Int(count))
            silence.pointee = ObjCBool(available == 0)
            return noErr
        }
        engine.attach(source)
        // The node converts its Int16 render format to the mixer's Float32;
        // the engine converts 32 kHz to the device/manual output sample rate.
        engine.connect(source, to: engine.mainMixerNode,
                       format: AVAudioFormat(standardFormatWithSampleRate: 32000, channels: 2)!)
        self.queue = queue
        self.source = source
        self.engine = engine
        self.offline = offlineSampleRate != nil
        if !offline { observeLifecycle() }
    }

    /// Returns accepted stereo frames; the producer retains any unwritten suffix.
    func enqueue(_ samples: UnsafePointer<Int16>, frames: Int) -> Int {
        guard frames >= 0, producer == nil else { return 0 }
        return melee_audio_ring_write(queue.pointer, samples, frames)
    }

    /// Pool/aux initialization and callback registration must precede this.
    /// Call from the control thread with any manual PCM producer stopped.
    func startMixer() throws {
        precondition(Thread.isMainThread)
        wantsMixer = true
        if interrupted { return }
        try startWorker()
    }

    private func startWorker() throws {
        guard producer == nil, let worker = melee_audio_producer_start(queue.pointer) else {
            throw NSError(domain: "MeleeAudio", code: 3,
                          userInfo: [NSLocalizedDescriptionKey: "Audio mixer is not ready or already owned"])
        }
        producer = worker
    }

    var diagnostics: [String: Any] {
        let stats = melee_audio_ring_stats(queue.pointer)
        var values: [String: Any] = ["requestedFrames": stats.requested_frames,
            "missingFrames": stats.missing_frames, "underruns": stats.underruns,
            "largestRequest": stats.largest_request]
        #if os(iOS)
        values["ioBufferDuration"] = AVAudioSession.sharedInstance().ioBufferDuration
        values["deviceSampleRate"] = AVAudioSession.sharedInstance().sampleRate
        #endif
        return values
    }

    var mixerStatus: Int32 { melee_audio_producer_status(producer) }

    func start() throws {
        precondition(Thread.isMainThread)
        wantsPlayback = true
        recoveryError = nil
        if interrupted { return }
        do { try resumeResources() }
        catch { recoveryError = error; stopResources(); throw error }
    }

    private func resumeResources() throws {
        if wantsMixer && producer == nil { try startWorker() }
        #if os(iOS)
        if !offline {
            try AVAudioSession.sharedInstance().setCategory(.playback, mode: .default)
            try AVAudioSession.sharedInstance().setActive(true)
            sessionActivated = true
        }
        #endif
        engine.prepare()
        do { try engine.start() }
        catch { stopResources(); throw error }
    }

    func stop() {
        precondition(Thread.isMainThread)
        wantsPlayback = false
        wantsMixer = false
        recoveryError = nil
        stopResources()
    }

    private func stopResources() {
        engine.stop()
        if let worker = producer {
            // Control-thread ownership avoids joining while inside an AX callback.
            precondition(melee_audio_producer_destroy(worker))
            producer = nil
        }
        melee_audio_ring_reset(queue.pointer)
        #if os(iOS)
        if sessionActivated {
            try? AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
            sessionActivated = false
        }
        #endif
    }
    /// Shared by system notifications and the offline lifecycle tests.
    func handle(_ event: Event) {
        precondition(Thread.isMainThread)
        switch event {
        case .interruptionBegan:
            interrupted = true
            stopResources()
            return
        case .interruptionEnded(let shouldResume):
            interrupted = false
            if !shouldResume { wantsPlayback = false }
        case .routeLost:
            wantsPlayback = false // Require explicit play after a disconnected output.
            stopResources()
            return
        case .configurationChanged:
            // Notifications queued during startup may arrive after recovery.
            // A real I/O format change has already stopped the engine.
            if engine.isRunning { return }
            stopResources()
        }
        guard wantsPlayback && !interrupted else { return }
        do {
            try resumeResources()
            recoveryError = nil
        } catch {
            recoveryError = error
            stopResources()
        }
    }

    private func observeLifecycle() {
        let center = NotificationCenter.default
        observers.append(center.addObserver(forName: .AVAudioEngineConfigurationChange,
                                             object: engine, queue: nil) { [weak self] _ in
            // Never tear down/join inside AVAudioEngine's notification queue.
            DispatchQueue.main.async { [weak self] in self?.handle(.configurationChanged) }
        })
        #if os(iOS)
        observers.append(center.addObserver(forName: AVAudioSession.interruptionNotification,
                                             object: nil, queue: nil) { [weak self] note in
            guard let raw = note.userInfo?[AVAudioSessionInterruptionTypeKey] as? UInt,
                  let type = AVAudioSession.InterruptionType(rawValue: raw) else { return }
            let options = (note.userInfo?[AVAudioSessionInterruptionOptionKey] as? UInt) ?? 0
            let event: Event = type == .began ? .interruptionBegan :
                .interruptionEnded(AVAudioSession.InterruptionOptions(rawValue: options).contains(.shouldResume))
            DispatchQueue.main.async { [weak self] in self?.handle(event) }
        })
        observers.append(center.addObserver(forName: AVAudioSession.routeChangeNotification,
                                             object: nil, queue: nil) { [weak self] note in
            guard let raw = note.userInfo?[AVAudioSessionRouteChangeReasonKey] as? UInt,
                  raw == AVAudioSession.RouteChangeReason.oldDeviceUnavailable.rawValue else { return }
            DispatchQueue.main.async { [weak self] in self?.handle(.routeLost) }
        })
        #endif
    }

    deinit {
        for token in observers { NotificationCenter.default.removeObserver(token) }
        stopResources()
    }
}
