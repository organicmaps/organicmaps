import UIKit

enum CarPlayWindowScaleAdjuster {
  static func updateAppearance(
    fromWindow sourceWindow: UIWindow?,
    toWindow destinationWindow: UIWindow?,
    isCarplayActivated: Bool
  ) {
    mapView?.graphicContextDidInitializeHandler = nil

    guard let destinationWindow else {
      // CarPlay can disconnect before the phone scene connects. Restore the default visual scale so
      // the lazily-created map is ready when the phone window eventually appears. If the graphics
      // context is not initialized, no CarPlay scale could have been applied and there is nothing to
      // restore; clearing the readiness handler above also cancels a pending CarPlay-scale update.
      if !isCarplayActivated, isGraphicContextInitialized {
        mapView?.updateVisualScaleToMain()
      }
      return
    }

    let sourceContentScale = sourceWindow?.traitCollection.displayScale
    let destinationContentScale = destinationWindow.traitCollection.displayScale

    // With a CarPlay-first launch there is no phone window to compare against, but the map was
    // created with the phone's default visual scale and still needs the CarPlay display scale.
    if let sourceContentScale, abs(sourceContentScale - destinationContentScale) <= 0.1 {
      return
    }
    if isCarplayActivated {
      updateWhenGraphicContextIsReady { mapView in
        mapView.updateVisualScale(to: destinationContentScale)
      }
    } else {
      updateWhenGraphicContextIsReady { mapView in
        mapView.updateVisualScaleToMain()
      }
    }
  }

  private static func updateWhenGraphicContextIsReady(_ update: @escaping (EAGLView) -> Void) {
    guard let mapView else { return }
    guard !mapView.graphicContextInitialized else {
      update(mapView)
      return
    }

    mapView.graphicContextDidInitializeHandler = { [weak mapView] in
      guard let mapView else { return }
      update(mapView)
    }
  }

  private static var isGraphicContextInitialized: Bool {
    mapView?.graphicContextInitialized ?? false
  }

  private static var mapView: EAGLView? {
    MapViewController.shared()?.mapView
  }
}
