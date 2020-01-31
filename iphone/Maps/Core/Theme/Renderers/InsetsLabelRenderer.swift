extension InsetsLabel {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        InsetsLabelRenderer.render(self, style: style)
    }
  }
}

class InsetsLabelRenderer: UILabelRenderer {
  class func render(_ control: InsetsLabel, style: Style) {
    super.render(control, style: style)
    if let insets = style.textContainerInset {
      control.insets = insets
    }
  }
}
