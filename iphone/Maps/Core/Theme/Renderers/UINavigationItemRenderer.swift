import UIKit
extension UINavigationItem {
  @objc func applyTheme() {
    assertionFailure("Can't apply on non UIView")
  }
}

class UINavigationItemRenderer {
  class func render(_ control: UINavigationItem, style: Style) {
    if let item = control.backBarButtonItem {
      UIBarButtonItemRenderer.render(item, style: style)
    }
    if let item = control.leftBarButtonItem {
      UIBarButtonItemRenderer.render(item, style: style)
    }
    if let item = control.rightBarButtonItem {
      UIBarButtonItemRenderer.render(item, style: style)
    }
  }
}
