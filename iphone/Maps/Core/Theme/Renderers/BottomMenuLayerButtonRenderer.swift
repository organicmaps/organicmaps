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
    if let titleLabel = control.titleLabel {
      if let font = style.font {
        titleLabel.font = font
      }
    }

    if let fontColor = style.fontColor {
      control.setTitleColor(fontColor, for: .normal)
    }

    if let imageView = control.imageView {
      UIImageViewRenderer.render(imageView, style: style)
    }
  }
}
