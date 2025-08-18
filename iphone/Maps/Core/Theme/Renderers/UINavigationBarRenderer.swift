
extension UINavigationBar {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      setStyle(.navigationBar)
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UINavigationBarRenderer.render(self, style: style)
    }
  }
}

class UINavigationBarRenderer: UIViewRenderer {
  class func render(_ control: UINavigationBar, style: Style) {
    super.render(control, style: style)
    if let barTintColor = style.barTintColor {
      if #available(iOS 13.0, *) {
        let appearance = UINavigationBarAppearance()
        appearance.configureWithOpaqueBackground()
        appearance.backgroundColor = barTintColor
        control.standardAppearance = appearance
        control.scrollEdgeAppearance = appearance
      } else {
        control.barTintColor = barTintColor
      }
    }
    if let shadowImage = style.shadowImage {
      if #available(iOS 13.0, *) {
        control.standardAppearance.shadowImage = shadowImage
        control.scrollEdgeAppearance!.shadowImage = shadowImage
      } else {
        control.shadowImage = shadowImage
      }
    }

    var attributes = [NSAttributedString.Key: Any]()
    if let font = style.font {
      attributes[NSAttributedString.Key.font] = font
    }
    if let fontColor = style.fontColor {
      attributes[NSAttributedString.Key.foregroundColor] = fontColor
    }
    if #available(iOS 13.0, *) {
      control.standardAppearance.titleTextAttributes = attributes
      control.scrollEdgeAppearance!.titleTextAttributes = attributes
    } else {
      control.titleTextAttributes = attributes
    }
  }
}
