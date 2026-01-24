final class TrafficButtonArea: AvailableArea {
  override func isAreaAffectingView(_ other: UIView) -> Bool {
    !other.trafficButtonAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.trafficButtonAreaAffectView
    let directions = ov.trafficButtonAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMTrafficButtonViewController.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var trafficButtonAreaAffectDirections: MWMAvailableAreaAffectDirections { [] }

  var trafficButtonAreaAffectView: UIView { self }
}
