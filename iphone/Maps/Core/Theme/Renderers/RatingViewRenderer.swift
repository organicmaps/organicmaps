import Foundation
extension RatingView {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        RatingViewRenderer.render(self, style: style)
    }
  }
}

class RatingViewRenderer {
  class func render(_ control: RatingView, style: Style) {
    if let settings = style.ratingViewSettings {
      control.settings = settings
    }
    if let borderWidth = style.borderWidth {
      control.borderWidth = borderWidth
    }
  }
}

