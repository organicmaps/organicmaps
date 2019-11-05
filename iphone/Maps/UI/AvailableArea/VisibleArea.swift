final class VisibleArea: AvailableArea {
  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.visibleAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.visibleAreaAffectView
    let directions = ov.visibleAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    if #available(iOS 12.0, *) {
      if CarPlayService.shared.isCarplayActivated {
        return
      }
    }
    FrameworkHelper.setVisibleViewport(areaFrame, scaleFactor: MapViewController.shared()?.mapView.contentScaleFactor ?? 1)
  }
}

extension UIView {
  @objc var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var visibleAreaAffectView: UIView { return self }
}
