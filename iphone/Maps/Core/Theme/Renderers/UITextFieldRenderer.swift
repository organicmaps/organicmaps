extension UITextField {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName) {
      UITextFieldRenderer.render(self, style: style)
    }
  }

  @objc override func sw_didMoveToWindow() {
    guard UIApplication.shared.keyWindow === window else {
      sw_didMoveToWindow();
      return
    }
    applyTheme()
    isStyleApplied = true
    sw_didMoveToWindow();
  }
}

class UITextFieldRenderer {
  class func render(_ control: UITextField, style: Style) {
    if let backgroundColor = style.backgroundColor {
      control.backgroundColor = backgroundColor
    }
    if let font = style.font {
      control.font = font
    }
    if let fontColor = style.fontColor {
      control.textColor = fontColor
    }
  }
}
