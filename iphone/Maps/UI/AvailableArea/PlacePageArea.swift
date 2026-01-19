final class PlacePageArea: AvailableArea {
  override var areaFrame: CGRect {
    frame
  }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    !other.placePageAreaAffectDirections.isEmpty
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
  @objc var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections { [] }

  var placePageAreaAffectView: UIView { self }
}
