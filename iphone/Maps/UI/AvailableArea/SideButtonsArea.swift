final class SideButtonsArea: AvailableArea {
  override var deferNotification: Bool { false }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    !other.sideButtonsAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.sideButtonsAreaAffectView
    let directions = ov.sideButtonsAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMSideButtons.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections { [] }

  var sideButtonsAreaAffectView: UIView { self }
}
