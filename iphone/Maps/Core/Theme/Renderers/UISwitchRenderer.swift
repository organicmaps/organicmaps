extension UISwitch {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "Switch"
    }
    for style in StyleManager.instance().getStyle(styleName) {
      UIViewRenderer.render(self, style: style)
      UISwitchRenderer.render(self, style: style)
    }
  }
}

class UISwitchRenderer {
  class func render(_ control: UISwitch, style: Style) {
    if let onTintColor = style.onTintColor {
      control.onTintColor = onTintColor
    }
  }
}
