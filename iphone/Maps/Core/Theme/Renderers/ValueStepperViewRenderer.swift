extension ValueStepperView {
  override func applyTheme() {
    if styleName.isEmpty {
      setStyle(.valueStepperView)
    }
    for style in StyleManager.shared.getStyle(styleName) where !style.isEmpty && !style.hasExclusion(view: self) {
      ValueStepperViewRenderer.render(self, style: style)
    }
  }
}

private final class ValueStepperViewRenderer {
  class func render(_ control: ValueStepperView, style: Style) {
    control.plusButton.coloring = style.coloring!
    control.minusButton.coloring = style.coloring!
    control.valueLabel.font = style.fontStyle?.font
    control.valueLabel.adjustsFontForContentSizeCategory = style.fontStyle?.isDynamic ?? false
    control.valueLabel.textColor = style.fontColor
  }
}
