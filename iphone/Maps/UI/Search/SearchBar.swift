import UIKit

class SearchBar: SolidTouchView {

  override var visibleAreaAffectDirection: VisibleArea.Direction { return val(iPhone: .top, iPad: .left) }
}
