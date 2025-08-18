final class PlacePageArea: AvailableArea {
  override var areaFrame: CGRect {
    return frame
  }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.placePageAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.placePageAreaAffectView
    let directions = ov.placePageAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMPlacePageManagerHelper.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var placePageAreaAffectView: UIView { return self }
}
