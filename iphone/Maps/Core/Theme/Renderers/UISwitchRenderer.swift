extension UISwitch {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "Switch"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UISwitchRenderer.render(self, style: style)
    }
  }
}

class UISwitchRenderer: UIViewRenderer {
  class func render(_ control: UISwitch, style: Style) {
    super.render(control, style: style)
    if let onTintColor = style.onTintColor {
      control.onTintColor = onTintColor
    }
  }
}
