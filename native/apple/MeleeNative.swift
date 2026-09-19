import SwiftUI
import Combine
import GameController
import UniformTypeIdentifiers

let actionNames = ["Move left", "Move right", "Move down", "Move up",
    "C left", "C right", "C down", "C up", "A · Attack", "B · Special",
    "X · Jump", "Y · Jump", "Z · Grab", "L · Shield", "R · Shield", "Start"]

@MainActor final class InputModel: ObservableObject {
    @Published var pads = [PADStatus](repeating: PADStatus(), count: 4)
    @Published var gamePads = [MeleeGamePad](repeating: MeleeGamePad(), count: 4)
    @Published var controllerNames = [String]()
    @Published var capture: Int?
    @Published var importing = false
    @Published var bindings: [String: UInt32] = [:]
    @Published var archiveSummary = "Choose an extracted .dat file to inspect its archive."
    var keyboard = MeleeKeyboard()
    var touch = PADStatus()
    var active = true
    private let preferences: UserDefaults
    private var controllers = [GCController?](repeating: nil, count: 4)
    private let deviceControl = ProcessInfo.processInfo.environment["MELEE_DEVICE_CONTROL"] == "1"
    private var probePoll: TimeInterval = 0
    private var probeDeadline: TimeInterval = 0
    private var probeSequence = -1
    private var probeInitialized = false
    private var probePad = PADStatus()

    // Explicitly enabled, app-container-only pad injection for tethered device
    // runtime checks. This is not evidence of physical touchscreen delivery.
    private func deviceTestPad() -> PADStatus {
        guard deviceControl else { return PADStatus() }
        let now = ProcessInfo.processInfo.systemUptime
        if now - probePoll >= 0.1 {
            probePoll = now
            struct Command: Decodable {
                let sequence: Int
                let buttons: UInt16
                let x: Float
                let y: Float
                let duration: Double
                let metalFXScale: Int?
            }
            let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
                .appendingPathComponent("Diagnostics", isDirectory: true)
            let data = try? Data(contentsOf: directory.appendingPathComponent("command.json"))
            if !probeInitialized {
                probeInitialized = true
                probeSequence = data.flatMap { try? JSONDecoder().decode(Command.self, from: $0) }?.sequence ?? -1
                return PADStatus()
            }
            if let data,
               let command = try? JSONDecoder().decode(Command.self, from: data),
               command.sequence > probeSequence, command.duration.isFinite,
               (0...3).contains(command.duration), (-1...1).contains(command.x), (-1...1).contains(command.y) {
                if let scale = command.metalFXScale {
                    guard [0, 2, 3].contains(scale) else { return PADStatus() }
                    preferences.set(scale, forKey: "metalfxScale")
                }
                probeSequence = command.sequence
                probePad = melee_analog_pad(command.buttons, command.x, command.y, 0, 0, 0, 0)
                probeDeadline = now + command.duration
                let acknowledgement: [String: Any] = ["sequence": probeSequence, "uptime": now]
                if let bytes = try? JSONSerialization.data(withJSONObject: acknowledgement) {
                    try? bytes.write(to: directory.appendingPathComponent("input-ack.json"), options: .atomic)
                }
                try? Data(String(probeSequence).utf8).write(
                    to: directory.appendingPathComponent("input-ack.txt"), options: .atomic)
            }
        }
        return now < probeDeadline ? probePad : PADStatus()
    }

    init(preferences: UserDefaults = .standard) {
        self.preferences = preferences
        do {
            let directory = try FileManager.default.url(for: .applicationSupportDirectory,
                in: .userDomainMask, appropriateFor: nil, create: true)
                .appendingPathComponent("MeleeNative", isDirectory: true)
            try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
            let path = directory.appendingPathComponent("system-settings.bin").path
            if !melee_settings_open(path) {
                archiveSummary = "Could not load system settings. Check the app’s storage before starting the game."
            }
        } catch {
            archiveSummary = "Could not create system settings storage: \(error.localizedDescription)"
        }
        #if !MELEE_GAME_RUNTIME
        melee_hsd_input_init()
        #endif
        melee_keyboard_defaults(&keyboard)
        if let data = preferences.data(forKey: "keyboardBindings"),
           let saved = try? JSONDecoder().decode([String: UInt32].self, from: data),
           saved.count == 128, saved.allSatisfy({ key, value in
               guard let code = UInt32(key) else { return false }
               return code < 128 && value < (1 << 16)
           }) {
            for (key, actions) in saved { _ = melee_keyboard_bind(&keyboard, UInt32(key)!, actions) }
        }
        refreshBindings()
    }

    func refreshBindings() {
        bindings = withUnsafeBytes(of: keyboard.bindings) { raw in
            let values = raw.bindMemory(to: UInt32.self)
            return Dictionary(uniqueKeysWithValues: (0..<128).map { (String($0), values[$0]) })
        }
    }

    func keyLabel(_ action: Int) -> String {
        let names = [0:"A", 1:"S", 2:"D", 3:"F", 4:"H", 5:"G", 6:"Z", 7:"X", 8:"C", 9:"V",
            11:"B", 12:"Q", 13:"W", 14:"E", 15:"R", 16:"Y", 17:"T", 18:"1", 19:"2",
            20:"3", 21:"4", 22:"6", 23:"5", 24:"=", 25:"9", 26:"7", 27:"−", 28:"8", 29:"0",
            30:"]", 31:"O", 32:"U", 33:"[", 34:"I", 35:"P", 36:"Return", 37:"L", 38:"J",
            39:"'", 40:"K", 41:";", 42:"\\", 43:",", 44:"/", 45:"N", 46:"M", 47:".",
            48:"Tab", 49:"Space", 50:"`", 51:"Delete", 53:"Escape", 65:"Keypad .",
            67:"Keypad *", 69:"Keypad +", 71:"Clear", 75:"Keypad /", 76:"Keypad Enter",
            78:"Keypad −", 81:"Keypad =", 82:"Keypad 0", 83:"Keypad 1", 84:"Keypad 2",
            85:"Keypad 3", 86:"Keypad 4", 87:"Keypad 5", 88:"Keypad 6", 89:"Keypad 7",
            91:"Keypad 8", 92:"Keypad 9", 96:"F5", 97:"F6", 98:"F7", 99:"F3", 100:"F8",
            101:"F9", 103:"F11", 109:"F10", 111:"F12", 114:"Help", 115:"Home", 116:"Page Up",
            117:"Forward Delete", 118:"F4", 119:"End", 120:"F2", 121:"Page Down", 122:"F1",
            123:"←", 124:"→", 125:"↓", 126:"↑"]
        let labels = bindings.compactMap { key, value -> Int? in
            value & (1 << action) != 0 ? Int(key) : nil
        }.sorted().map { names[$0] ?? "Key \($0)" }
        return labels.isEmpty ? "Unassigned" : labels.joined(separator: "/")
    }

    func persistBindings() {
        refreshBindings()
        if let data = try? JSONEncoder().encode(bindings) {
            preferences.set(data, forKey: "keyboardBindings")
        }
    }

    func resetBindings() { melee_keyboard_defaults(&keyboard); capture = nil; persistBindings() }

    func key(_ code: UInt16, pressed: Bool, repeated: Bool = false, commandModified: Bool = false) {
        guard code < 128 else { return }
        if commandModified {
            // A key pressed before Command must still be released, even when
            // its key-up also belongs to a normal macOS shortcut.
            if !pressed { melee_keyboard_event(&keyboard, UInt32(code), false) }
            return
        }
        if let action = capture {
            if pressed && !repeated {
                if code != 53 { // Escape cancels capture.
                    for (key, value) in bindings {
                        _ = melee_keyboard_bind(&keyboard, UInt32(key)!, value & ~(1 << action))
                    }
                    _ = melee_keyboard_bind(&keyboard, UInt32(code), 1 << action)
                    persistBindings()
                }
                capture = nil
                melee_keyboard_clear(&keyboard)
            }
            return
        }
        melee_keyboard_event(&keyboard, UInt32(code), pressed)
    }

    func clear() {
        melee_keyboard_clear(&keyboard); touch = PADStatus(); capture = nil
        probeDeadline = 0; probePad = PADStatus()
        melee_pad_disconnect_all()
    }

    func sample(connected available: [GCController] = GCController.controllers()) {
        let connected = available.filter { $0.extendedGamepad != nil }
        for i in 0..<4 {
            if let c = controllers[i], !connected.contains(where: { $0 === c }) { controllers[i] = nil }
        }
        for c in connected where !controllers.contains(where: { $0 === c }) {
            if let slot = controllers.firstIndex(where: { $0 == nil }) { controllers[slot] = c }
        }
        controllerNames = controllers.enumerated().compactMap { i, c in
            c.map { "P\(i + 1): \($0.vendorName ?? "Game controller")" }
        }
        var next = [PADStatus](repeating: PADStatus(), count: 4)
        for i in 0..<4 {
            next[i].err = -1
            guard active, let g = controllers[i]?.extendedGamepad else { continue }
            var buttons: UInt16 = 0
            let mapping: [(GCControllerButtonInput, UInt16)] = [
                (g.buttonA, 0x100), (g.buttonB, 0x200), (g.buttonX, 0x400), (g.buttonY, 0x800),
                (g.leftShoulder, 0x40), (g.rightShoulder, 0x10), (g.buttonMenu, 0x1000),
                (g.dpad.left, 1), (g.dpad.right, 2), (g.dpad.down, 4), (g.dpad.up, 8)]
            for (button, mask) in mapping where button.isPressed { buttons |= mask }
            if g.leftTrigger.value > 0.95 { buttons |= 0x40 }
            if g.rightTrigger.value > 0.95 { buttons |= 0x20 }
            next[i] = melee_analog_pad(buttons, g.leftThumbstick.xAxis.value,
                g.leftThumbstick.yAxis.value, g.rightThumbstick.xAxis.value,
                g.rightThumbstick.yAxis.value, g.leftTrigger.value, g.rightTrigger.value)
        }
        if active && capture == nil {
            next[0] = melee_pad_merge(next[0], melee_pad_merge(melee_keyboard_read(&keyboard),
                melee_pad_merge(touch, deviceTestPad())))
        }
        if touch.button != 0 && ProcessInfo.processInfo.environment["MELEE_UI_INPUT_TRACE"] == "1" { NSLog("Melee sampled touch %x merged %x active %d", Int(touch.button), Int(next[0].button), active ? 1 : 0) }
        pads = next
        next.withUnsafeBufferPointer { melee_pad_publish($0.baseAddress!, 0) }
        #if !MELEE_GAME_RUNTIME
        var processed = [MeleeGamePad](repeating: MeleeGamePad(), count: 4)
        processed.withUnsafeMutableBufferPointer { melee_hsd_input_frame($0.baseAddress!) }
        gamePads = processed
        #endif
    }

    func inspect(_ url: URL) {
        let access = url.startAccessingSecurityScopedResource()
        defer { if access { url.stopAccessingSecurityScopedResource() } }
        do {
            let data = try Data(contentsOf: url, options: .mappedIfSafe)
            archiveSummary = data.withUnsafeBytes { raw in
                var archive = MeleeArchive()
                guard melee_archive_open(&archive, raw.baseAddress, raw.count) else {
                    return "\(url.lastPathComponent): invalid or unsupported HSD archive."
                }
                var lines = ["\(url.lastPathComponent): \(archive.data_size) data bytes, \(archive.reloc_count) relocations"]
                for index in 0..<min(archive.public_count, 20) {
                    var name: UnsafePointer<CChar>?
                    var offset: UInt32 = 0
                    if melee_archive_public(&archive, index, &name, &offset), let name {
                        lines.append("\(String(cString: name)) @ \(offset)")
                    }
                }
                return lines.joined(separator: "\n")
            }
        } catch { archiveSummary = error.localizedDescription }
    }
}

#if !MELEE_TEST
@main struct MeleeNativeApp: App {
    @StateObject private var input = InputModel()
    #if MELEE_GAME_RUNTIME && os(macOS)
    @NSApplicationDelegateAdaptor(GameAppDelegate.self) private var delegate
    #endif
    var body: some Scene {
        #if MELEE_GAME_RUNTIME && os(macOS)
        Window("Melee", id: "game") { GameView(input: input) }
        #else
        WindowGroup {
            #if MELEE_GAME_RUNTIME
            GameView(input: input)
            #else
            PortView(input: input)
            #endif
        }
        #endif
    }
}
#endif

struct PortView: View {
    @ObservedObject var input: InputModel
    @Environment(\.scenePhase) private var phase
    private let timer = Timer.publish(every: 1.0 / 60, on: .main, in: .common).autoconnect()
    var body: some View {
        #if os(iOS)
        VStack(alignment: .leading, spacing: 6) {
            Text("Melee · Native ARM port").font(.headline)
            Text("Platform test build — gameplay is not linked yet.").font(.caption).foregroundStyle(.secondary)
            Text(String(format: "P1  Stick %d,%d  C %d,%d  Buttons %04X  L %d  R %d",
                input.pads[0].stickX, input.pads[0].stickY, input.pads[0].substickX,
                input.pads[0].substickY, input.pads[0].button, input.pads[0].triggerLeft,
                input.pads[0].triggerRight)).font(.system(.caption, design: .monospaced))
            Text(String(format: "HSD  Stick %.2f,%.2f  L %.2f R %.2f  Press %08X Release %08X",
                input.gamePads[0].stick_x, input.gamePads[0].stick_y, input.gamePads[0].left,
                input.gamePads[0].right, input.gamePads[0].trigger, input.gamePads[0].release))
                .font(.system(.caption2, design: .monospaced))
            TouchSurface(input: input).frame(minHeight: 160, maxHeight: .infinity)
            HStack {
                Button("Inspect archive…") { input.clear(); input.importing = true }
                Text(input.controllerNames.joined(separator: " · ")).font(.caption)
            }
            ScrollView { Text(input.archiveSummary).font(.caption).frame(maxWidth: .infinity, alignment: .leading) }
                .frame(height: 28)
        }.padding(12)
            .onReceive(timer) { _ in input.sample() }
            .onChange(of: phase) { value in input.active = value == .active; if !input.active { input.clear() } }
            .fileImporter(isPresented: $input.importing, allowedContentTypes: [.data]) { result in
                if case .success(let url) = result { input.inspect(url) }
            }
        #else
        content
            .onReceive(timer) { _ in input.sample() }
            .onChange(of: phase) { value in input.active = value == .active; if !input.active { input.clear() } }
            .fileImporter(isPresented: $input.importing, allowedContentTypes: [.data]) { result in
                if case .success(let url) = result { input.inspect(url) }
            }
        #endif
    }
    private var content: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Melee · Native ARM port").font(.title.bold())
            Text("Platform test build — gameplay is not linked yet.").foregroundStyle(.secondary)
            Text(input.controllerNames.isEmpty ? "No paired game controller detected" : input.controllerNames.joined(separator: " · "))
            HStack(spacing: 24) {
                StickView(title: "Movement", x: input.pads[0].stickX, y: input.pads[0].stickY)
                StickView(title: "C stick", x: input.pads[0].substickX, y: input.pads[0].substickY)
                VStack(alignment: .leading) {
                    Text(String(format: "Buttons  %04X", input.pads[0].button)).monospaced()
                    Text("L \(input.pads[0].triggerLeft)  R \(input.pads[0].triggerRight)").monospaced()
                    Text(String(format: "HSD stick %.2f,%.2f · L %.2f R %.2f",
                        input.gamePads[0].stick_x, input.gamePads[0].stick_y,
                        input.gamePads[0].left, input.gamePads[0].right)).font(.caption.monospaced())
                    Text("Controller: A/B/X/Y → A/B/X/Y\nRight shoulder → Grab\nTriggers → Shield").font(.caption)
                }
            }
            #if os(macOS)
            Text(input.capture == nil ? "WASD or arrows · J attack · K special · U/I jump · Q/E shield · Space grab · Return start" : "Press a key to bind; Escape cancels.")
                .font(.callout)
            ScrollView {
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 220))], alignment: .leading) {
                    ForEach(0..<actionNames.count, id: \.self) { action in
                        Button {
                            input.clear(); input.capture = action
                        } label: {
                            HStack { Text(actionNames[action]); Spacer(); Text(keyLabel(action)) }
                        }.buttonStyle(.bordered)
                    }
                }
            }.frame(maxHeight: 190)
            Button("Restore default bindings") { input.resetBindings() }
            #else
            TouchSurface(input: input).frame(minHeight: 210)
            #endif
            Divider()
            Button("Inspect game archive…") { input.clear(); input.importing = true }
            ScrollView { Text(input.archiveSummary).font(.system(.caption, design: .monospaced)).frame(maxWidth: .infinity, alignment: .leading) }
                .frame(maxHeight: 110)
        }
        .padding(24)
        #if os(macOS)
        .frame(minWidth: 740, minHeight: 600)
        .background(KeyboardSurface(input: input))
        #endif
    }

    func keyLabel(_ action: Int) -> String { input.keyLabel(action) }
}

struct StickView: View {
    let title: String
    let x: Int8
    let y: Int8
    var body: some View {
        VStack {
            ZStack {
                Circle().stroke(.secondary, lineWidth: 2).frame(width: 90, height: 90)
                Circle().fill(.blue).frame(width: 15, height: 15).offset(x: CGFloat(x) / 2, y: -CGFloat(y) / 2)
            }
            Text(title).font(.caption)
        }
    }
}

#if os(macOS)
import AppKit
struct KeyboardSurface: NSViewRepresentable {
    let input: InputModel
    func makeNSView(context: Context) -> KeyView { KeyView(input: input) }
    func updateNSView(_ view: KeyView, context: Context) {}
}

final class KeyView: NSView {
    let input: InputModel
    var monitor: Any?
    init(input: InputModel) { self.input = input; super.init(frame: .zero) }
    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }
    override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()
        if let monitor { NSEvent.removeMonitor(monitor); self.monitor = nil }
        guard window != nil else { return }
        monitor = NSEvent.addLocalMonitorForEvents(matching: [.keyDown, .keyUp]) { [weak self] event in
            guard let self, event.window === self.window, self.window?.isKeyWindow == true,
                  self.window?.attachedSheet == nil else { return event }
            let commandModified = event.modifierFlags.contains(.command)
            self.input.key(event.keyCode, pressed: event.type == .keyDown,
                           repeated: event.isARepeat, commandModified: commandModified)
            return commandModified ? event : nil
        }
    }
    deinit { if let monitor { NSEvent.removeMonitor(monitor) } }
}
#else
import UIKit
struct TouchSurface: UIViewRepresentable {
    let input: InputModel
    var gameOverlay = false
    func makeUIView(context: Context) -> TouchView { TouchView(input: input, gameOverlay: gameOverlay) }
    func updateUIView(_ view: TouchView, context: Context) { if !input.active { view.clear() } }
}

final class TouchView: UIView {
    let input: InputModel
    let gameOverlay: Bool
    let traceInput = ProcessInfo.processInfo.environment["MELEE_UI_INPUT_TRACE"] == "1"
    var points: [ObjectIdentifier: CGPoint] = [:]
    var roles: [ObjectIdentifier: Int] = [:]
    let labels = ["Move", "C", "A", "B", "X", "Y", "Z", "L", "R", "Start"]
    let masks: [UInt16] = [0, 0, 0x100, 0x200, 0x400, 0x800, 0x10, 0x40, 0x20, 0x1000]
    init(input: InputModel, gameOverlay: Bool = false) {
        self.input = input; self.gameOverlay = gameOverlay; super.init(frame: .zero)
        isMultipleTouchEnabled = true; backgroundColor = .clear
        if gameOverlay {
            isAccessibilityElement = true
            accessibilityIdentifier = "melee.game.controls"
            accessibilityLabel = "Game controls"
            accessibilityTraits = .allowsDirectInteraction
        }
    }
    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }
    func regions() -> [CGRect] {
        let w = bounds.width, h = bounds.height
        if gameOverlay {
            let stick = min(48.0, w * 0.065, h * 0.20)
            let button = min(23.0, w * 0.032, h * 0.085)
            let centers = [CGPoint(x: w * 0.09, y: h * 0.60), CGPoint(x: w * 0.23, y: h * 0.77),
                CGPoint(x: w * 0.87, y: h * 0.65), CGPoint(x: w * 0.95, y: h * 0.46),
                CGPoint(x: w * 0.79, y: h * 0.46), CGPoint(x: w * 0.87, y: h * 0.27),
                CGPoint(x: w * 0.78, y: h * 0.86), CGPoint(x: w * 0.09, y: h * 0.10),
                CGPoint(x: w * 0.97, y: h * 0.08), CGPoint(x: w * 0.50, y: h * 0.94)]
            return centers.enumerated().map { i, center in
                let radius = i < 2 ? stick : button
                let x = max(radius + 2, min(w - radius - 2, center.x))
                let y = max(radius + 2, min(h - radius - 2, center.y))
                return CGRect(x: x-radius, y: y-radius, width: radius*2, height: radius*2)
            }
        }
        let r = min(48.0, w * 0.105)
        let centers = [CGPoint(x: w * 0.13, y: h * 0.53), CGPoint(x: w * 0.38, y: h * 0.53),
            CGPoint(x: w * 0.76, y: h * 0.7), CGPoint(x: w * 0.91, y: h * 0.52),
            CGPoint(x: w * 0.61, y: h * 0.52), CGPoint(x: w * 0.76, y: h * 0.32),
            CGPoint(x: w * 0.6, y: h * 0.9), CGPoint(x: w * 0.13, y: h * 0.08),
            CGPoint(x: w * 0.91, y: h * 0.08), CGPoint(x: w * 0.4, y: h * 0.9)]
        return centers.enumerated().map { i, c in
            let radius = i < 2 ? r : min(23, w * 0.06)
            let y = max(radius + 2, min(h - radius - 2, c.y))
            return CGRect(x: c.x-radius, y: y-radius, width: radius*2, height: radius*2)
        }
    }
    override func draw(_ rect: CGRect) {
        for (i, r) in regions().enumerated() {
            if gameOverlay {
                UIColor.black.withAlphaComponent(0.65).setFill()
                UIBezierPath(ovalIn: r).fill()
            }
            (roles.values.contains(i) ? UIColor.systemBlue :
                (gameOverlay ? UIColor.white.withAlphaComponent(0.8) : UIColor.secondaryLabel)).setStroke()
            let path = UIBezierPath(ovalIn: r); path.lineWidth = 2; path.stroke()
            let attrs: [NSAttributedString.Key: Any] = [.font: UIFont.systemFont(ofSize: 13, weight: .semibold), .foregroundColor: gameOverlay ? UIColor.white : UIColor.label]
            let text = labels[i] as NSString, size = text.size(withAttributes: attrs)
            text.draw(at: CGPoint(x: r.midX-size.width/2, y: r.midY-size.height/2), withAttributes: attrs)
        }
    }
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for t in touches {
            let id = ObjectIdentifier(t), point = t.location(in: self)
            if traceInput { NSLog("Melee touch began %@ bounds %@ active %d", String(describing: point), String(describing: bounds), input.active ? 1 : 0) }
            if let role = regions().firstIndex(where: { $0.contains(point) }) {
                if role < 2 && roles.values.contains(role) { continue }
                roles[id] = role; points[id] = point
            }
        }
        updatePad()
    }
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for t in touches where roles[ObjectIdentifier(t)] != nil { points[ObjectIdentifier(t)] = t.location(in: self) }
        updatePad()
    }
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) { release(touches) }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) { release(touches) }
    func release(_ touches: Set<UITouch>) {
        for t in touches { roles.removeValue(forKey: ObjectIdentifier(t)); points.removeValue(forKey: ObjectIdentifier(t)) }
        updatePad()
    }
    func clear() { if traceInput && !roles.isEmpty { NSLog("Melee touch clear with active roles") }; roles.removeAll(); points.removeAll(); updatePad() }
    override func layoutSubviews() { super.layoutSubviews(); clear() }
    func updatePad() {
        var axes = [Float](repeating: 0, count: 4), buttons: UInt16 = 0
        let rects = regions()
        for (id, role) in roles {
            guard let p = points[id] else { continue }
            if role < 2 {
                let r = rects[role]
                axes[role*2] = Float((p.x-r.midX)/(r.width/2))
                axes[role*2+1] = Float((r.midY-p.y)/(r.height/2))
            } else { buttons |= masks[role] }
        }
        if traceInput { NSLog("Melee touch pad buttons %x", Int(buttons)) }
        input.touch = melee_analog_pad(buttons, axes[0], axes[1], axes[2], axes[3],
            buttons & 0x40 != 0 ? 1 : 0, buttons & 0x20 != 0 ? 1 : 0)
        setNeedsDisplay()
    }
}
#endif
