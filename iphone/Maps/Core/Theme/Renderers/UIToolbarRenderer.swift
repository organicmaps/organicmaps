extension UIToolbar {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName) {
      UIToolbarRenderer.render(self, style: style)
    }
  }
}

class UIToolbarRenderer {
  class func render(_ control: UIToolbar, style: Style) {
    if let backgroundColor = style.backgroundColor {
      control.setBackgroundImage(backgroundColor.getImage(), forToolbarPosition: .any, barMetrics: .default)
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
  }
}
