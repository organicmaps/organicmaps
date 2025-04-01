final class SearchOnMapAreaView: UIView {

  override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {}

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: .bottom, iPad: [])
  }

  override var trafficButtonAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: .bottom, iPad: [])
  }
}
