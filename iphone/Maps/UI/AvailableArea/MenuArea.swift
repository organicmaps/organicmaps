final class MenuArea: AvailableArea {
  override var areaFrame: CGRect {
    return frame
  }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.menuAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.menuAreaAffectView
    let directions = ov.menuAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMBottomMenuViewController.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var menuAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var menuAreaAffectView: UIView { return self }
}
