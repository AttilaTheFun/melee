import XCTest

/// Physical-device test: requires the user's disc already in Documents/Melee.ciso.
/// Runs only when explicitly included in a locally signed test scheme.
final class DeviceGraphicsTests: XCTestCase {
    func testUpscalingAndWidescreen() throws {
        continueAfterFailure = false
        let app = XCUIApplication()
        XCUIDevice.shared.orientation = .landscapeLeft
        app.launchEnvironment["MELEE_UI_FRAME_CLOCK_TEST"] = "1"
        app.launchEnvironment["MELEE_DEVICE_DIAGNOSTICS"] = "1"
        app.launch()
        defer { app.terminate() }
        let frames = app.staticTexts["melee.test.frameSequence"]
        XCTAssertTrue(frames.waitForExistence(timeout: 30))
        func progress() {
            let before = UInt64(frames.label) ?? 0
            let predicate = NSPredicate { _, _ in (UInt64(frames.label) ?? 0) > before + 120 }
            expectation(for: predicate, evaluatedWith: frames)
            waitForExpectations(timeout: 30)
        }
        func reveal(_ element: XCUIElement) {
            for _ in 0..<5 {
                if element.exists && element.isHittable { return }
                app.collectionViews.firstMatch.swipeUp()
            }
            XCTAssertTrue(element.isHittable)
        }
        func configure(wide: Bool, scale: String) {
            app.buttons["melee.settings"].tap()
            XCTAssertTrue(app.buttons["Done"].waitForExistence(timeout: 5))
            let toggle = app.switches["melee.widescreen"]
            reveal(toggle)
            if (toggle.value as? String == "1") != wide {
                // The accessibility element spans the full Form row. Tap the
                // actual trailing switch rather than the row's label.
                toggle.coordinate(withNormalizedOffset: CGVector(dx: 0.95, dy: 0.5)).tap()
            }
            XCTAssertEqual(toggle.value as? String, wide ? "1" : "0")
            let picker = app.segmentedControls.firstMatch
            reveal(picker)
            XCTAssertTrue(picker.buttons[scale].isEnabled)
            picker.buttons[scale].tap()
            XCTAssertTrue(picker.buttons[scale].isSelected)
            app.buttons["Done"].tap()
            progress()
            XCTAssertEqual(app.staticTexts["melee.test.aspect"].label, wide ? "16:9" : "4:3")
            XCTAssertEqual(app.staticTexts["melee.test.metalState"].label, scale == "Off" ? "1" : "0")
            let capture = XCTAttachment(screenshot: XCUIScreen.main.screenshot())
            capture.name = "\(wide ? "16-9" : "4-3")-\(scale)"
            capture.lifetime = .keepAlways; add(capture)
        }
        progress()
        configure(wide: false, scale: "2×")
        configure(wide: false, scale: "3×")
        configure(wide: true, scale: "3×")
        configure(wide: true, scale: "Off")
        let copies = app.staticTexts["melee.test.readbacks"].label
        progress()
        XCTAssertEqual(app.staticTexts["melee.test.readbacks"].label, copies)
        configure(wide: true, scale: "2×")
        app.terminate(); app.launch()
        XCTAssertTrue(frames.waitForExistence(timeout: 30)); progress()
        XCTAssertEqual(app.staticTexts["melee.test.aspect"].label, "16:9")
        XCTAssertEqual(app.staticTexts["melee.test.metalState"].label, "0")
        configure(wide: false, scale: "Off")
    }
}
