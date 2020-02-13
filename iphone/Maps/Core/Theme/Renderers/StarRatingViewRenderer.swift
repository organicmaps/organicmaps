extension StarRatingView {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "StarRatingView"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        StarRatingViewViewRenderer.render(self, style: style)
    }
  }
}

class StarRatingViewViewRenderer {
  class func render(_ control: StarRatingView, style: Style) {
    if let onTintColor = style.onTintColor {
      control.activeColor = onTintColor
    }
    if let offTintColor = style.offTintColor {
      control.inactiveColor = offTintColor
    }
  }
}

