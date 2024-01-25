import UIKit

class BottomTabBarView: SolidTouchView {
  
  @IBOutlet var mainButtonsView: UIView!
  
  override var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: [], iPad: [.bottom])
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return [.bottom]
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return [.bottom]
  }

  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    guard super.point(inside: point, with: event), mainButtonsView.point(inside: convert(point, to: mainButtonsView), with: event) else { return false }
    for subview in mainButtonsView.subviews {
      if !subview.isHidden
          && subview.isUserInteractionEnabled
          && subview.point(inside: convert(point, to: subview), with: event) {
        return true
      }
    }
    return false
  }
}
