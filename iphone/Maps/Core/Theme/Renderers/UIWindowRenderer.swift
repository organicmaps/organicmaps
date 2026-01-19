extension UIWindow {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UIWindowRenderer.render(self, style: style)
    }
  }

  @objc func sw_becomeKeyWindow() {
    if !isStyleApplied {
      applyTheme()
    }
    isStyleApplied = true
    sw_becomeKeyWindow()
  }
}

class UIWindowRenderer {
  class func render(_: UIView, style _: Style) {}
}
