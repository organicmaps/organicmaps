import Foundation
extension Checkmark {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "Checkmark"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        CheckmarkRenderer.render(self, style: style)
    }
  }
}

class CheckmarkRenderer {
  class func render(_ control: Checkmark, style: Style) {
    if let onTintColor = style.onTintColor {
      control.onTintColor = onTintColor
    }

    if let offTintColor = style.offTintColor {
      control.offTintColor = offTintColor
    }
  }
}

