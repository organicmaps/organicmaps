import Chart

extension ChartView {
  override func applyTheme() {
    if styleName.isEmpty {
      styleName = "ChartView"
    }
    for style in StyleManager.shared.getStyle(styleName) where !style.isEmpty && !style.hasExclusion(view: self) {
      ChartViewRenderer.render(self, style: style)
    }
  }
}

fileprivate final class ChartViewRenderer {
  class func render(_ control: ChartView, style: Style) {
    control.backgroundColor = style.backgroundColor
    control.textColor = style.fontColor!
    control.font = style.font!
    control.gridColor = style.gridColor!
    control.previewSelectorColor = style.previewSelectorColor!
    control.previewTintColor = style.previewTintColor!
    control.infoBackgroundColor = style.infoBackground!
    control.infoShadowColor = style.shadowColor!
    control.infoShadowOpacity = style.shadowOpacity!
  }
}
