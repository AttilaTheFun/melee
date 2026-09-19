import AVFAudio

@main
struct AudioOutputTest {
    static func main() throws {
        for rate in [32000.0, 48000.0] {
            let output = try MeleeAudioOutput(offlineSampleRate: rate)
            let samples = (0..<4096).flatMap { _ in [Int16(8192), Int16(-4096)] }
            samples.withUnsafeBufferPointer {
                precondition(output.enqueue($0.baseAddress!, frames: 4096) == 4096)
            }
            try output.start()
            let format = output.engine.manualRenderingFormat
            let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: 512)!
            let status = try output.engine.renderOffline(512, to: buffer)
            precondition(status == .success && buffer.frameLength == 512)
            let channels = buffer.floatChannelData!
            // Skip the rate converter's startup transient when changing rates.
            for i in (rate == 32000 ? 0 : 128)..<512 {
                precondition(abs(channels[0][i] - 0.25) < 0.0001)
                precondition(abs(channels[1][i] + 0.125) < 0.0001)
            }
            // Drain beyond the queued PCM and converter tail: underrun is silence.
            for _ in 0..<20 { _ = try output.engine.renderOffline(512, to: buffer) }
            for i in 0..<Int(buffer.frameLength) {
                precondition(abs(channels[0][i]) < 0.0001 && abs(channels[1][i]) < 0.0001)
            }
            output.stop()
        }
        // Connect the real worker to the same source node. An empty voice pool
        // produces silence; nonzero voice PCM/pacing is verified by the C test.
        melee_ax_voice_pool_init()
        precondition(melee_ax_aux_init())
        let live = try MeleeAudioOutput(offlineSampleRate: 32000)
        for _ in 0..<2 {
            try live.startMixer()
            precondition(live.mixerStatus == MELEE_AUDIO_PRODUCING)
            try live.start()
            let buffer = AVAudioPCMBuffer(pcmFormat: live.engine.manualRenderingFormat, frameCapacity: 160)!
            let status = try live.engine.renderOffline(160, to: buffer)
            precondition(status == .success)
            for channel in 0..<2 { for i in 0..<160 {
                precondition(buffer.floatChannelData![channel][i] == 0)
            } }
            live.stop()
            precondition(live.mixerStatus == MELEE_AUDIO_STOPPED)
        }
        try live.startMixer(); try live.start()
        live.handle(.interruptionBegan)
        precondition(!live.engine.isRunning && live.mixerStatus == MELEE_AUDIO_STOPPED)
        live.handle(.configurationChanged)
        precondition(!live.engine.isRunning && live.mixerStatus == MELEE_AUDIO_STOPPED)
        live.handle(.interruptionEnded(true))
        precondition(live.engine.isRunning && live.mixerStatus == MELEE_AUDIO_PRODUCING)
        live.engine.stop() // Model the documented hardware-change behavior.
        live.handle(.configurationChanged)
        precondition(live.engine.isRunning && live.recoveryError == nil)
        live.handle(.interruptionBegan); live.stop(); live.handle(.interruptionEnded(true))
        precondition(!live.engine.isRunning && live.mixerStatus == MELEE_AUDIO_STOPPED)
        try live.startMixer(); try live.start()
        live.handle(.interruptionBegan); live.handle(.interruptionEnded(false))
        precondition(!live.engine.isRunning)
        try live.start(); precondition(live.engine.isRunning)
        live.handle(.routeLost); live.handle(.configurationChanged)
        precondition(!live.engine.isRunning && live.mixerStatus == MELEE_AUDIO_STOPPED)
        try live.start(); precondition(live.engine.isRunning)
        live.handle(.interruptionBegan)
        let competing = try MeleeAudioOutput(offlineSampleRate: 32000)
        try competing.startMixer()
        live.handle(.interruptionEnded(true))
        precondition(live.recoveryError != nil && !live.engine.isRunning)
        competing.stop()
        try live.start()
        precondition(live.recoveryError == nil && live.engine.isRunning)
        live.stop()
        print("Apple offline audio: conversion, mixer start/stop, interruption intent and configuration recovery passed")
    }
}
