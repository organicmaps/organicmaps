extension UILabel {
  var isAttributed: Bool {
    guard let attributedText = attributedText else {
      return false
    }
    guard !attributedText.string.isEmpty else {
      return false
    }
    var range = NSRange()
    attributedText.attributes(at: 0, effectiveRange: &range)
    return attributedText.string.count != range.length
  }

  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UILabelRenderer.render(self, style: style)
    }
  }

  @objc func sw_setAttributedText(text: NSAttributedString) -> CGRect {
    var attributedString = text
    if styleName.isEmpty == false {
      let styles = StyleManager.shared.getStyle(styleName)
      for style in styles where style.attributes != nil {
        attributedString = UILabelRenderer.transformText(style: style,
                                                         text: attributedString)
      }
    }
    return self.sw_setAttributedText(text: attributedString)
  }
}

class UILabelRenderer: UIViewRenderer {
  class func render(_ control: UILabel, style: Style) {
    super.render(control, style: style)
    if let font = style.font {
      control.font = font
    }
    if let fontColor = style.fontColor {
      control.textColor = fontColor
    }
    if let textAlignment = style.textAlignment {
      control.textAlignment = textAlignment
    }
    if style.attributes != nil,
      control.isAttributed,
      let attributedText = control.attributedText {
      control.attributedText = attributedText
    }
  }

  class func transformText(style: Style, text: NSAttributedString) -> NSAttributedString {
    if let attributes = style.attributes,
      attributes.isEmpty == false,
      let attributedtext = text.mutableCopy() as? NSMutableAttributedString{
      attributedtext.setAttributes(attributes, range: NSRange(location: 0, length: attributedtext.length));
      return attributedtext
    }
    return text
  }
}
