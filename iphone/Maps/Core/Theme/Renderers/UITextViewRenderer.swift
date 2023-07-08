extension UITextView {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UITextViewRenderer.render(self, style: style)
    }
  }
}

class UITextViewRenderer {
  class func render(_ control: UITextView, style: Style) {
    if let backgroundColor = style.backgroundColor {
      control.backgroundColor = backgroundColor
    }
    if let font = style.font {
      control.font = font
    }
    if let fontColor = style.fontColor {
      control.textColor = fontColor
    }
    if let textContainerInset = style.textContainerInset {
      control.textContainerInset = textContainerInset
    }
    if let linkAttributes = style.linkAttributes {
      control.linkTextAttributes = linkAttributes
    }
  }
}
