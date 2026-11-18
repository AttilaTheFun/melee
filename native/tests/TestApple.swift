import Foundation
import GameController
import Combine

@main struct TestApple {
    @MainActor static func main() {
        let suite = "dev.melee.native.tests.\(UUID().uuidString)"
        let prefs = UserDefaults(suiteName: suite)!
        defer { prefs.removePersistentDomain(forName: suite) }
        let input = InputModel(preferences: prefs)
        precondition(input.keyLabel(0) == "A/←" && input.keyLabel(8) == "J")
        // Repeated polling while Settings is open must not rebuild its controls.
        input.sample(connected: [], deliver: false)
        var padChanges = 0, nameChanges = 0
        let padObserver = input.$pads.dropFirst().sink { _ in padChanges += 1 }
        let nameObserver = input.$controllerNames.dropFirst().sink { _ in nameChanges += 1 }
        for _ in 0..<120 { input.sample(connected: [], deliver: false) }
        precondition(padChanges == 0 && nameChanges == 0)
        padObserver.cancel(); nameObserver.cancel()
        // Both advertised movement layouts must produce the same game input,
        // including releasing one of two keys bound to the same direction.
        for (letter, arrow, x, y) in [(0,123,-80,0), (2,124,80,0),
                                      (1,125,0,-80), (13,126,0,80)] {
            for code in [letter, arrow] {
                input.key(UInt16(code), pressed: true); input.sample(connected: [])
                precondition(input.pads[0].stickX == x && input.pads[0].stickY == y)
                input.key(UInt16(code), pressed: false); input.sample(connected: [])
                precondition(input.pads[0].stickX == 0 && input.pads[0].stickY == 0)
            }
            input.key(UInt16(letter), pressed: true)
            input.key(UInt16(arrow), pressed: true)
            input.key(UInt16(letter), pressed: false); input.sample(connected: [])
            precondition(input.pads[0].stickX == x && input.pads[0].stickY == y)
            input.clear(); input.sample(connected: [])
            precondition(input.pads[0].stickX == 0 && input.pads[0].stickY == 0)
        }
        let defaults = input.bindings
        input.capture = 0
        input.key(7, pressed: true, repeated: true)
        precondition(input.capture == 0 && input.bindings == defaults)
        input.key(53, pressed: true)
        precondition(input.capture == nil && input.bindings == defaults)
        precondition(prefs.data(forKey: "keyboardBindings") == nil)
        input.key(13, pressed: true)
        input.sample(connected: [])
        precondition(input.pads[0].stickY == 80)
        precondition(input.gamePads[0].stick_y == 1)
        input.key(13, pressed: false, commandModified: true)
        input.sample(connected: [])
        precondition(input.pads[0].stickY == 0 && input.gamePads[0].stick_y == 0)
        input.key(13, pressed: true, commandModified: true)
        input.sample(connected: [])
        precondition(input.pads[0].stickY == 0)
        input.clear(); input.capture = 8
        input.key(7, pressed: true, commandModified: true)
        input.key(7, pressed: false, commandModified: true)
        precondition(input.capture == 8 && input.bindings["7"] == 0)
        input.key(7, pressed: true)
        precondition(input.capture == nil && input.bindings["7"] == 1 << 8)
        let restored = InputModel(preferences: prefs)
        precondition(restored.bindings["7"] == 1 << 8 && restored.bindings["38"] == 0)
        precondition(restored.keyLabel(8) == "X")
        restored.key(7, pressed: true); restored.sample(connected: [])
        precondition(restored.pads[0].button == 0x100)
        precondition(restored.gamePads[0].button & 0x100 != 0)
        precondition(restored.gamePads[0].trigger & 0x100 != 0)
        restored.sample(connected: [])
        precondition(restored.gamePads[0].trigger == 0)
        restored.active = false; restored.clear(); restored.sample(connected: [])
        precondition(restored.pads[0].button == 0 && restored.pads[0].err == -1)
        precondition(restored.gamePads[0].error == -1 && restored.gamePads[0].release & 0x100 != 0)
        restored.active = true
        let c1 = GCController.withExtendedGamepad()
        let c2 = GCController.withExtendedGamepad()
        c1.extendedGamepad!.buttonB.setValue(1)
        c1.extendedGamepad!.leftThumbstick.setValueForXAxis(0.5, yAxis: 0)
        c1.extendedGamepad!.leftTrigger.setValue(0.5)
        c2.extendedGamepad!.buttonX.setValue(1)
        restored.touch = melee_analog_pad(0x100, 1, 0, 0, 0, 0, 0)
        precondition(!restored.hasPlayerOneController)
        restored.sample(connected: [c1, c2])
        precondition(restored.hasPlayerOneController && restored.touch.button == 0 && restored.touch.stickX == 0)
        precondition(restored.pads[0].button == 0x200 && restored.pads[0].stickX == 40)
        precondition(restored.pads[1].button == 0x400)
        precondition(restored.gamePads[0].stick_x == 0.5 && restored.gamePads[0].left == 0.5)
        precondition(restored.gamePads[1].trigger & 0x400 != 0)
        restored.sample(connected: [c2, c1])
        precondition(restored.pads[0].button == 0x200 && restored.pads[1].button == 0x400)
        restored.sample(connected: [c2])
        precondition(!restored.hasPlayerOneController)
        precondition(restored.pads[0].button == 0 && restored.pads[1].button == 0x400)
        restored.bindController(1, to: 0x100)
        restored.bindController(6, to: 0x20)
        restored.sample(connected: [c1])
        precondition(restored.pads[0].button & 0x100 != 0 && restored.pads[0].button & 0x200 == 0)
        precondition(restored.gamePads[0].right == 0.5 && restored.gamePads[0].left == 0)
        restored.sample(connected: [c1], deliver: false)
        precondition(restored.hasPlayerOneController && restored.pads[0].button == 0 && restored.pads[0].stickX == 0)
        let controllerReload = InputModel(preferences: prefs)
        precondition(controllerReload.controllerBindings[1] == 0x100 && controllerReload.controllerBindings[6] == 0x20)
        controllerReload.resetControllerBindings()
        precondition(controllerReload.controllerBindings == ControllerBindings.defaults)
        restored.resetBindings()
        precondition(restored.bindings["0"] == 1 && restored.bindings["123"] == 1)
        print("Apple input model: WASD/arrows, capture cancellation, remapping, persistence, controller slots, disconnect and focus tests passed.")
    }
}
