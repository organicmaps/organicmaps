extension UIToolbar {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UIToolbarRenderer.render(self, style: style)
    }
  }
}

class UIToolbarRenderer {
  class func render(_ control: UIToolbar, style: Style) {
    if let backgroundColor = style.backgroundColor {
      let traits = control.window?.traitCollection ?? control.traitCollection
      control.setBackgroundImage(backgroundColor.getImage(traits), forToolbarPosition: .any, barMetrics: .default)
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
  }
}
