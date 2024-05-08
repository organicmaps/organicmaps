import Foundation
import UIKit

enum CarPlayWindowScaleAdjuster {

  static func updateAppearance(
    fromWindow sourceWindow: UIWindow,
    toWindow destinationWindow: UIWindow,
    isCarplayActivated: Bool
  ) {

    let sourceContentScale = sourceWindow.screen.scale;
    let destinationContentScale = destinationWindow.screen.scale;

    if abs(sourceContentScale - destinationContentScale) > 0.1 {
      if isCarplayActivated {
        updateVisualScale(to: destinationContentScale)
      } else {
        updateVisualScaleToMain()
      }
    }
  }

  private static func updateVisualScale(to scale: CGFloat) {
    if isGraphicContextInitialized {
      mapViewController?.mapView.updateVisualScale(to: scale)
    } else {
      DispatchQueue.main.async {
        updateVisualScale(to: scale)
      }
    }
  }

  private static func updateVisualScaleToMain() {
    if isGraphicContextInitialized {
      mapViewController?.mapView.updateVisualScaleToMain()
    } else {
      DispatchQueue.main.async {
        updateVisualScaleToMain()
      }
    }
  }

  private static var isGraphicContextInitialized: Bool {
    return mapViewController?.mapView.graphicContextInitialized ?? false
  }

  private static var mapViewController: MapViewController? {
    return MapViewController.shared()
  }
}
