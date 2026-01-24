import UIKit

class ExpandedTouchView: UIView {
  override func point(inside point: CGPoint, with _: UIEvent?) -> Bool {
    let rect = bounds.insetBy(dx: -30, dy: 0)
    return rect.contains(point)
  }
}
