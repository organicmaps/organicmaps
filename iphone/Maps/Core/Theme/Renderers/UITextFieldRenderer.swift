extension UITextField {
  @objc override func applyTheme() {
    for style in StyleManager.instance().getStyle(styleName) {
      UITextFieldRenderer.render(self, style: style)
    }
  }

  @objc func sw_textRect(forBounds bounds: CGRect) -> CGRect {
    if !isStyleApplied {
      applyTheme()
    }
    isStyleApplied = true
    return self.sw_textRect(forBounds: bounds)
  }

  @objc func sw_editingRect(bounds: CGRect) -> CGRect {
    return self.textRect(forBounds: bounds)
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
