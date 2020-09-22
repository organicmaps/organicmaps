final class GuidesNavigationBarArea: AvailableArea {
  override var deferNotification: Bool { return false }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.guidesNavigationBarAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.guidesNavigationBarAreaAffectView
    let directions = ov.guidesNavigationBarAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    GuidesNavigationBarViewController.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var guidesNavigationBarAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var guidesNavigationBarAreaAffectView: UIView { return self }
}
