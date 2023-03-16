extension BottomMenuLayerButton {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        BottomMenuLayerButtonRenderer.render(self, style: style)
    }
  }
}

class BottomMenuLayerButtonRenderer {
  class func render(_ control: BottomMenuLayerButton, style: Style) {
    if let font = style.font {
      control.titleLabel.font = font
    }

    if let fontColor = style.fontColor {
      control.titleLabel.textColor = fontColor
    }

    UIImageViewRenderer.render(control.imageView, style: style)
  }
}
