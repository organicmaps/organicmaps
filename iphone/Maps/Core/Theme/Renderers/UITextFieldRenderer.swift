extension UITextField {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UITextFieldRenderer.render(self, style: style)
    }
  }

  @objc override func sw_didMoveToWindow() {
    guard MapsAppDelegate.theApp().window === window else {
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
    var placeholderAttributes = [NSAttributedString.Key : Any]()
    if let backgroundColor = style.backgroundColor {
      control.backgroundColor = backgroundColor
    }
    if let font = style.font {
      control.font = font
      placeholderAttributes[NSAttributedString.Key.font] = font
    }
    if let fontColor = style.fontColor {
      control.textColor = fontColor

    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
      placeholderAttributes[NSAttributedString.Key.foregroundColor] = tintColor
    }
    if let attributedPlaceholder = control.attributedPlaceholder, !attributedPlaceholder.string.isEmpty {
      control.attributedPlaceholder = NSAttributedString(string: attributedPlaceholder.string,
                                                         attributes: placeholderAttributes)
    }
  }
}
