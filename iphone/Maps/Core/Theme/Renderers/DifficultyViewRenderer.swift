import Foundation
extension DifficultyView {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "DifficultyView"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        DifficultyViewRenderer.render(self, style: style)
    }
  }
}

class DifficultyViewRenderer: UIViewRenderer {
  class func render(_ control: DifficultyView, style: Style) {
    super.render(control, style: style)
    if let colors = style.colors {
      control.colors = colors
    }
    if let emptyColor = style.offTintColor {
      control.emptyColor = emptyColor
    }
  }
}

