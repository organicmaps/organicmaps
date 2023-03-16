final class WidgetsArea: AvailableArea {
  override var areaFrame: CGRect {
    return alternative(iPhone: {
      var frame = super.areaFrame
      frame.origin.y -= 16
      frame.size.height += 16
      return frame
    }, iPad: { super.areaFrame })()
  }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.widgetsAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.widgetsAreaAffectView
    let directions = ov.widgetsAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMMapWidgetsHelper.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var widgetsAreaAffectView: UIView { return self }
}
