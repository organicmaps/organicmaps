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
    MWMFrameworkHelper.setVisibleViewport(frame)
  }
}

extension UIView {
  @objc var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var visibleAreaAffectView: UIView { return self }
}
