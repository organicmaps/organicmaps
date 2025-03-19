final class SearchOnMapAreaView: UIView {
  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: .bottom, iPad: [])
  }

  override var trafficButtonAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: .bottom, iPad: [])
  }
}
