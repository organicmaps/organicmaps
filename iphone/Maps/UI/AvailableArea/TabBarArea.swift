final class TabBarArea: AvailableArea {
  override var areaFrame: CGRect {
    return frame
  }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.tabBarAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.tabBarAreaAffectView
    let directions = ov.tabBarAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    BottomTabBarViewController.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var tabBarAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var tabBarAreaAffectView: UIView { return self }
}
