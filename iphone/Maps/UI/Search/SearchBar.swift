import UIKit

class SearchBar: SolidTouchView {
  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: .top, iPad: .left) }
}
