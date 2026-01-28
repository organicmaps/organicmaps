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
    if CarPlayService.shared.isCarplayActivated {
      return
    }
    let availableAreaFrame = areaFrame
    guard !skipAvailableAreaUpdate(for: availableAreaFrame) else {
      return
    }
    FrameworkHelper.setVisibleViewport(availableAreaFrame, scaleFactor: MapViewController.shared()?.mapView.contentScaleFactor ?? 1.0)
  }

  private func skipAvailableAreaUpdate(for area: CGRect) -> Bool {
    let mainScreenBounds = UIScreen.main.bounds
    let kMinAvailableToUpdateFactor: CGFloat = 4.0
    return area.height < mainScreenBounds.height / kMinAvailableToUpdateFactor ||
           area.width < mainScreenBounds.width / kMinAvailableToUpdateFactor
  }
}

extension UIView {
  @objc var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var visibleAreaAffectView: UIView { return self }
}
