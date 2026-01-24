final class TrackRecordingButtonArea: AvailableArea {
  override func isAreaAffectingView(_ other: UIView) -> Bool {
    !other.trackRecordingButtonAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.trackRecordingButtonAreaAffectView
    let directions = ov.trackRecordingButtonAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    TrackRecordingButtonViewController.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var trackRecordingButtonAreaAffectDirections: MWMAvailableAreaAffectDirections { [] }

  var trackRecordingButtonAreaAffectView: UIView { self }
}
