final class VisibleArea: AvailableArea {
  override func filterAffectingViews(_ other: UIView) -> Bool {
    return !other.visibleAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.visibleAreaAffectView
    let directions = ov.visibleAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver(_ rect: CGRect) {
    MWMFrameworkHelper.setVisibleViewport(rect)
  }
}

extension UIView {
  var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var visibleAreaAffectView: UIView { return self }
}
