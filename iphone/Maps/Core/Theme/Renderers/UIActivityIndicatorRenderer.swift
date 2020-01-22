extension UIActivityIndicatorView {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName) {
      UIActivityIndicatorRenderer.render(self, style: style)
    }
  }
}

class UIActivityIndicatorRenderer {
  class func render(_ control: UIActivityIndicatorView, style: Style) {
    if let color = style.color {
      control.color = color
    }
  }
}
