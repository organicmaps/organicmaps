import UIKit

class BottomTabBarView: SolidTouchView {
  override var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: [], iPad: [.bottom])
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return [.bottom]
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return [.bottom]
  }
}
