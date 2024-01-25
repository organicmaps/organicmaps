final class TabBarArea: AvailableArea {
  override var areaFrame: CGRect {
    var areaFrame = frame
    // Spacing is used only for devices with zero bottom safe area (such as SE).
    let additionalBottomSpacing: CGFloat = MapsAppDelegate.theApp().window.safeAreaInsets.bottom.isZero ? -10 : .zero
    areaFrame.origin.y += additionalBottomSpacing
    return areaFrame
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
