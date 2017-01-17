import UIKit

class SearchBar: SolidTouchView {

  override var visibleAreaAffectDirection: VisibleArea.Direction { return IPAD() ? .left : .top }
}
