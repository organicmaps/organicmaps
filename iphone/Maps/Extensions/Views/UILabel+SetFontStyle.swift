extension UILabel {
  func setFontStyle(_ font: FontStyleSheet, color: TextColorStyleSheet? = nil) {
    var name = font.rawValue
    if let color {
      name += ":\(color.rawValue)"
    }
    styleName = name
  }

  func setFontStyle(_ color: TextColorStyleSheet) {
    styleName = color.rawValue
  }

  func setFontStyleAndApply(_ font: FontStyleSheet, color: TextColorStyleSheet? = nil) {
    setFontStyle(font, color: color)
    applyTheme()
  }

  func setFontStyleAndApply(_ color: TextColorStyleSheet) {
    setFontStyle(color)
    applyTheme()
  }
}
