import CarPlay

/// Drape derives its visual scale from the phone screen. While the map is shown on a CarPlay display
/// with a different pixel density it must use the CarPlay scale, and get the phone scale back when the
/// map returns to the phone.
enum CarPlayWindowScaleAdjuster {
  /// The phone scale to return to, set while the CarPlay scale is applied.
  private static var phoneScale: CGFloat?

  static func applyCarPlayScale(_ window: CPWindow) {
    guard let mapView else { return }
    // displayScale is 0 until the trait collection is resolved. There is no retry, so log it: the map
    // would silently stay at the phone scale for the whole CarPlay session.
    let carPlayScale = window.traitCollection.displayScale
    guard carPlayScale >= 1.0 else {
      LOG(.warning, "Unresolved CarPlay display scale, the map is left at the phone scale.")
      return
    }
    // Not mapView.contentScaleFactor: the map view is already hosted by the CarPlay window here, while
    // the engine was created with the scale EAGLView takes from the main screen.
    let phoneContentScale = UIScreen.main.nativeScale
    guard abs(carPlayScale - phoneContentScale) > 0.1 else { return }

    // The map may still be creating its Drape engine (e.g. on a CarPlay-first launch): defer the update
    // until the graphics context exists.
    if mapView.graphicContextInitialized {
      apply(carPlayScale, phoneScale: phoneContentScale, to: mapView)
    } else {
      mapView.graphicContextDidInitializeHandler = { [weak mapView] in
        if let mapView {
          apply(carPlayScale, phoneScale: phoneContentScale, to: mapView)
        }
      }
    }
  }

  static func restorePhoneScale() {
    guard let mapView else { return }
    // Cancel a CarPlay-scale update that is still waiting for the graphics context.
    mapView.graphicContextDidInitializeHandler = nil
    guard let scale = phoneScale else { return }
    phoneScale = nil
    mapView.updateVisualScale(withContentScaleFactor: scale)
  }

  private static func apply(_ carPlayScale: CGFloat, phoneScale: CGFloat, to mapView: EAGLView) {
    self.phoneScale = phoneScale
    mapView.updateVisualScale(withContentScaleFactor: carPlayScale)
  }

  private static var mapView: EAGLView? {
    MapViewController.shared()?.mapView
  }
}
