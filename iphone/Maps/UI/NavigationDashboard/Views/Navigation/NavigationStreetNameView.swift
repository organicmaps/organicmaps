final class NavigationStreetNameView: SolidTouchView {
  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: [], iPad: .top)
  }

  override var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: [], iPad: .top)
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    .top
  }

  override var trackRecordingButtonAreaAffectDirections: MWMAvailableAreaAffectDirections {
    .top
  }
}
