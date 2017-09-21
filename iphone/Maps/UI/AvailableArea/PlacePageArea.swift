final class PlacePageArea: AvailableArea {
  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.placePageAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.placePageAreaAffectView
    let directions = ov.placePageAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMPlacePageManagerHelper.updateAvailableArea(frame)
  }
}

extension UIView {
  @objc var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var placePageAreaAffectView: UIView { return self }
}
