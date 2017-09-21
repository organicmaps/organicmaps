final class WidgetsArea: AvailableArea {
  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.widgetsAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.widgetsAreaAffectView
    let directions = ov.widgetsAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMMapWidgetsHelper.updateAvailableArea(frame)
  }
}

extension UIView {
  @objc var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var widgetsAreaAffectView: UIView { return self }
}
