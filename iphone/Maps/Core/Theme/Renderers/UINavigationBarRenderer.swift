
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
      let appearance = UINavigationBarAppearance()
      appearance.configureWithOpaqueBackground()
      appearance.backgroundColor = barTintColor
      control.standardAppearance = appearance
      control.scrollEdgeAppearance = appearance
    }
    if let shadowImage = style.shadowImage {
      control.standardAppearance.shadowImage = shadowImage
      control.scrollEdgeAppearance!.shadowImage = shadowImage
    }

    var attributes = [NSAttributedString.Key: Any]()
    if let font = style.font {
      attributes[NSAttributedString.Key.font] = font
    }
    if let fontColor = style.fontColor {
      attributes[NSAttributedString.Key.foregroundColor] = fontColor
    }
    control.standardAppearance.titleTextAttributes = attributes
    control.scrollEdgeAppearance!.titleTextAttributes = attributes
  }
}
