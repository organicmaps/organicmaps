extension ValueStepperView {
  override func applyTheme() {
    if styleName.isEmpty {
      styleName = "ValueStepperView"
    }
    for style in StyleManager.shared.getStyle(styleName) where !style.isEmpty && !style.hasExclusion(view: self) {
      ValueStepperViewRenderer.render(self, style: style)
    }
  }
}

fileprivate final class ValueStepperViewRenderer {
  class func render(_ control: ValueStepperView, style: Style) {
    control.plusButton.coloring = style.coloring!
    control.minusButton.coloring = style.coloring!
    control.valueLabel.font = style.font
    control.valueLabel.textColor = style.fontColor
  }
}
