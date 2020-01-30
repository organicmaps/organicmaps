extension MWMButton {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        MWMButtonRenderer.render(self, style: style)
    }
  }
}

class MWMButtonRenderer {
  class func render(_ control: MWMButton, style: Style) {
    UIButtonRenderer.render(control, style: style)
    if let coloring = style.coloring {
      control.coloring = coloring
    }
    if let imageName = style.mwmImage {
      control.imageName = imageName
    }
  }
}

