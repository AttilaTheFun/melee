#if MELEE_GAME_RUNTIME
import SwiftUI
import QuartzCore

#if os(iOS)
import UIKit
struct DirectMetalSurface: UIViewRepresentable {
    func makeUIView(context: Context) -> MetalHost { MetalHost() }
    func updateUIView(_ view: MetalHost, context: Context) { view.publish() }
    static func dismantleUIView(_ view: MetalHost, coordinator: ()) { view.detach() }
    final class MetalHost: UIView {
        override class var layerClass: AnyClass { CAMetalLayer.self }
        override func didMoveToWindow() { super.didMoveToWindow(); publish() }
        override func layoutSubviews() { super.layoutSubviews(); publish() }
        func publish() {
            guard let window, bounds.width > 0, bounds.height > 0 else { detach(); return }
            let metal = layer as! CAMetalLayer
            metal.isOpaque = true; metal.contentsScale = window.screen.scale
            metal.presentsWithTransaction = false
            melee_runtime_set_metal_layer(Unmanaged.passUnretained(metal).toOpaque(),
                UInt32((bounds.width * metal.contentsScale).rounded()),
                UInt32((bounds.height * metal.contentsScale).rounded()))
        }
        func detach() { melee_runtime_set_metal_layer(nil, 0, 0) }
    }
}
#else
import AppKit
struct DirectMetalSurface: NSViewRepresentable {
    func makeNSView(context: Context) -> MetalHost { let view = MetalHost(); view.wantsLayer = true; return view }
    func updateNSView(_ view: MetalHost, context: Context) { view.publish() }
    static func dismantleNSView(_ view: MetalHost, coordinator: ()) { view.detach() }
    final class MetalHost: NSView {
        override func makeBackingLayer() -> CALayer { CAMetalLayer() }
        override func viewDidMoveToWindow() { super.viewDidMoveToWindow(); publish() }
        override func viewDidChangeBackingProperties() { super.viewDidChangeBackingProperties(); publish() }
        override func layout() { super.layout(); publish() }
        func publish() {
            guard let window, let metal = layer as? CAMetalLayer,
                  bounds.width > 0, bounds.height > 0 else { detach(); return }
            metal.isOpaque = true; metal.contentsScale = window.backingScaleFactor
            metal.presentsWithTransaction = false
            melee_runtime_set_metal_layer(Unmanaged.passUnretained(metal).toOpaque(),
                UInt32((bounds.width * metal.contentsScale).rounded()),
                UInt32((bounds.height * metal.contentsScale).rounded()))
        }
        func detach() { melee_runtime_set_metal_layer(nil, 0, 0) }
    }
}
#endif
#endif
