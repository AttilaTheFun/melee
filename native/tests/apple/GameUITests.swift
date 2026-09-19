import XCTest

final class GameUITests: XCTestCase {
    /// Opt-in, file-driven XCTest session. Commands are restricted to touch
    /// controls and screenshots in the simulated app; no shell or host input.
    func testControlSession() throws {
        guard let path = ProcessInfo.processInfo.environment["MELEE_UI_SESSION_DIR"] else {
            throw XCTSkip("Generate the project with --session-dir to enable simulator control")
        }
        continueAfterFailure = false
        let directory = URL(fileURLWithPath: path, isDirectory: true)
        let app = XCUIApplication()
        XCUIDevice.shared.orientation = .landscapeLeft
        app.launchEnvironment["MELEE_DISC_IMAGE"] = try XCTUnwrap(ProcessInfo.processInfo.environment["MELEE_UI_TEST_DISC"])
        app.launchEnvironment["MELEE_TEST_SILENT"] = "1"
        app.launchEnvironment["MELEE_UI_INPUT_TRACE"] = "1"
        app.launch()
        defer { app.terminate() }
        let controls = app.otherElements["melee.game.controls"]
        XCTAssertTrue(controls.waitForExistence(timeout: 20))
        // Record the actual startup scene before navigating. A fixed sequence
        // of A presses can outlast the title's ten-second input window and
        // enter attract-mode loading before the scripted Start press.
        Thread.sleep(forTimeInterval: 3)
        struct Command: Decodable {
            let sequence: Int
            let action: String
            var x: Double?
            var y: Double?
            var duration: Double?
            var button: String?
        }
        let buttons: [String: CGVector] = [
            "A": CGVector(dx: 0.87, dy: 0.65), "B": CGVector(dx: 0.95, dy: 0.46),
            "X": CGVector(dx: 0.79, dy: 0.46), "Y": CGVector(dx: 0.87, dy: 0.27),
            "Z": CGVector(dx: 0.78, dy: 0.86), "L": CGVector(dx: 0.09, dy: 0.10),
            "R": CGVector(dx: 0.97, dy: 0.08), "Start": CGVector(dx: 0.50, dy: 0.94),
        ]
        func record(_ sequence: Int) throws {
            try XCUIScreen.main.screenshot().pngRepresentation.write(
                to: directory.appendingPathComponent("frame-\(sequence).png"), options: .atomic)
            let state: [String: Any] = ["sequence": sequence, "running": app.state == .runningForeground,
                                        "controls": String(describing: controls.frame)]
            try JSONSerialization.data(withJSONObject: state).write(
                to: directory.appendingPathComponent("state.json"), options: .atomic)
        }
        var last = 0
        try record(last)
        let deadline = Date().addingTimeInterval(1800)
        while Date() < deadline {
            guard let data = try? Data(contentsOf: directory.appendingPathComponent("command.json")) else {
                Thread.sleep(forTimeInterval: 0.1); continue
            }
            let command = try JSONDecoder().decode(Command.self, from: data)
            if command.sequence <= last { Thread.sleep(forTimeInterval: 0.1); continue }
            let duration = command.duration ?? 0.15
            XCTAssertTrue((0...5).contains(duration))
            if command.action == "quit" { try record(command.sequence); return }
            if command.action == "press" {
                let point = try XCTUnwrap(buttons[command.button ?? ""])
                controls.coordinate(withNormalizedOffset: point).press(forDuration: duration)
            } else if command.action == "stick" {
                let x = command.x ?? 0, y = command.y ?? 0
                XCTAssertTrue((-1...1).contains(x) && (-1...1).contains(y))
                let bounds = controls.frame
                let radius = min(48.0, bounds.width * 0.065, bounds.height * 0.20)
                let center = controls.coordinate(withNormalizedOffset: CGVector(dx: 0.09, dy: 0.60))
                let destination = center.withOffset(CGVector(dx: x * radius, dy: -y * radius))
                center.press(forDuration: 0.02, thenDragTo: destination,
                             withVelocity: .fast, thenHoldForDuration: duration)
            } else {
                XCTAssertEqual(command.action, "snapshot")
            }
            Thread.sleep(forTimeInterval: 0.4)
            XCTAssertEqual(app.state, .runningForeground)
            last = command.sequence
            try record(last)
        }
        XCTFail("Simulator control session timed out")
    }

    func testFrameClockResumesAfterBackgroundAndSettings() throws {
        continueAfterFailure = false
        let app = XCUIApplication()
        XCUIDevice.shared.orientation = .landscapeLeft
        app.launchEnvironment["MELEE_DISC_IMAGE"] = try XCTUnwrap(ProcessInfo.processInfo.environment["MELEE_UI_TEST_DISC"])
        app.launchEnvironment["MELEE_TEST_SILENT"] = "1"
        app.launchEnvironment["MELEE_UI_FRAME_CLOCK_TEST"] = "1"
        app.launch()
        defer { app.terminate() }
        let counter = app.staticTexts["melee.test.frameSequence"]
        XCTAssertTrue(counter.waitForExistence(timeout: 20))
        func waitForProgress(after previous: UInt64) {
            let progress = NSPredicate { _, _ in (UInt64(counter.label) ?? 0) > previous + 10 }
            expectation(for: progress, evaluatedWith: counter)
            waitForExpectations(timeout: 20)
        }
        waitForProgress(after: 0)
        let metal = app.staticTexts["melee.test.metalState"]
        XCTAssertEqual(metal.label, "1", "The direct Metal surface must be active")
        let copies = app.staticTexts["melee.test.readbacks"]
        let initialCopies = copies.label
        let firstFrame = try XCTUnwrap(UInt64(counter.label))
        waitForProgress(after: firstFrame + 120)
        XCTAssertEqual(copies.label, initialCopies, "Direct presentation must not read pixels back to the CPU")
        let direct = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
        direct.name = "direct-metal-frame"; direct.lifetime = .keepAlways; add(direct)
        var before = try XCTUnwrap(UInt64(counter.label))
        XCUIDevice.shared.press(.home)
        XCTAssertTrue(app.wait(for: .runningBackground, timeout: 5))
        app.activate()
        waitForProgress(after: before)
        app.buttons["Graphics…"].tap()
        XCTAssertTrue(app.staticTexts["Graphics"].waitForExistence(timeout: 5))
        app.buttons["Done"].tap()
        before = try XCTUnwrap(UInt64(counter.label))
        waitForProgress(after: before)
        XCTAssertEqual(app.state, .runningForeground)
        XCTAssertEqual(metal.label, "1")
    }

    func testGraphicsAndTouchAppearance() throws {
        continueAfterFailure = false
        // Simulator appearance is set externally with simctl ui; a launch
        // preference does not reliably change UIKit traits.
        for appearance in [ProcessInfo.processInfo.environment["MELEE_UI_TEST_APPEARANCE"] ?? "Current"] {
            let app = XCUIApplication()
            XCUIDevice.shared.orientation = .landscapeLeft
            app.launchEnvironment["MELEE_DISC_IMAGE"] = try XCTUnwrap(ProcessInfo.processInfo.environment["MELEE_UI_TEST_DISC"])
            app.launchEnvironment["MELEE_TEST_SILENT"] = "1"
            app.launch()
            XCTAssertTrue(app.otherElements["melee.game.controls"].waitForExistence(timeout: 20))
            Thread.sleep(forTimeInterval: 3)
            app.buttons["Graphics…"].tap()
            XCTAssertTrue(app.staticTexts["Graphics"].waitForExistence(timeout: 5))
            XCTAssertTrue(app.staticTexts["MetalFX is unavailable on this device. The original image will be displayed."].exists)
            let options = app.segmentedControls.firstMatch
            XCTAssertTrue(options.exists)
            XCTAssertFalse(options.buttons.element(boundBy: 1).isEnabled)
            XCTAssertFalse(options.buttons.element(boundBy: 2).isEnabled)
            let settings = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
            settings.name = "graphics-fallback-\(appearance)"; settings.lifetime = .keepAlways; add(settings)
            app.buttons["Done"].tap()
            Thread.sleep(forTimeInterval: 2)
            XCTAssertEqual(app.state, .runningForeground)
            let screen = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
            screen.name = "touch-overlay-\(appearance)"; screen.lifetime = .keepAlways; add(screen)
            app.terminate()
        }
    }

    func testTouchAdvancesStartup() throws {
        continueAfterFailure = false
        let disc = try XCTUnwrap(ProcessInfo.processInfo.environment["MELEE_UI_TEST_DISC"])
        let app = XCUIApplication()
        XCUIDevice.shared.orientation = .landscapeLeft
        app.launchEnvironment["MELEE_DISC_IMAGE"] = disc
        app.launchEnvironment["MELEE_TEST_SILENT"] = "1"
        app.launch()
        XCTAssertTrue(app.staticTexts["Melee"].waitForExistence(timeout: 20))
        let controls = app.otherElements["melee.game.controls"]
        XCTAssertTrue(controls.waitForExistence(timeout: 10))
        print("Game app frame: \(app.frame), controls: \(controls.frame)")
        // Coordinates are relative to the actual controls view, independent
        // of device safe-area insets; events stay inside the simulator.
        let attack = controls.coordinate(withNormalizedOffset: CGVector(dx: 0.87, dy: 0.65))
        let start = controls.coordinate(withNormalizedOffset: CGVector(dx: 0.50, dy: 0.94))
        for step in 0..<5 {
            Thread.sleep(forTimeInterval: 2)
            let snapshot = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
            snapshot.name = "startup-\(step)"; snapshot.lifetime = .keepAlways
            add(snapshot)
            attack.press(forDuration: 0.15)
        }
        start.press(forDuration: 0.15)
        Thread.sleep(forTimeInterval: 2)
        XCTAssertEqual(app.state, .runningForeground)
        let final = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
        final.name = "after-touch-start"; final.lifetime = .keepAlways
        add(final)
        let move = controls.coordinate(withNormalizedOffset: CGVector(dx: 0.09, dy: 0.60))
        let down = controls.coordinate(withNormalizedOffset: CGVector(dx: 0.09, dy: 0.72))
        move.press(forDuration: 0.05, thenDragTo: down, withVelocity: .fast, thenHoldForDuration: 0.08)
        Thread.sleep(forTimeInterval: 1)
        let stick = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
        stick.name = "after-touch-stick"; stick.lifetime = .keepAlways; add(stick)
        attack.press(forDuration: 0.15)
        Thread.sleep(forTimeInterval: 1)
        attack.press(forDuration: 0.15)
        Thread.sleep(forTimeInterval: 3)
        let selection = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
        selection.name = "after-touch-versus"; selection.lifetime = .keepAlways; add(selection)
        XCTAssertEqual(app.state, .runningForeground)
        print("Game UI final screenshot checkpoint")
        Thread.sleep(forTimeInterval: 15)
        // Screenshots are reviewed against the rendered game screens. Running
        // and touch delivery alone do not prove navigation reached a menu.
        app.terminate()
    }
}
