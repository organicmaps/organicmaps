import UIKit

extension UINavigationBar {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "NavigationBar"
    }
    for style in StyleManager.instance().getStyle(styleName) {
      UIViewRenderer.render(self, style:  style)
      UINavigationBarRenderer.render(self, style: style)
    }
  }
}

class UINavigationBarRenderer {
  class func render(_ control: UINavigationBar, style: Style) {
    if let barTintColor = style.barTintColor {
      control.barTintColor = barTintColor
    }
    if let shadowImage = style.shadowImage {
      control.shadowImage = shadowImage
    }

    var attributes = [NSAttributedString.Key: Any]()
    if let font = style.font {
      attributes[NSAttributedString.Key.font] = font
    }
    if let fontColor = style.fontColor {
      attributes[NSAttributedString.Key.foregroundColor] = fontColor
    }
    control.titleTextAttributes = attributes
  }
}
