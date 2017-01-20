import UIKit

class SearchBar: SolidTouchView {

  override var visibleAreaAffectDirection: VisibleArea.Direction { return alternative(iPhone: .top, iPad: .left) }
}
