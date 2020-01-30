import Foundation
extension RatingSummaryView {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        RatingSummaryViewRenderer.render(self, style: style)
    }
  }
}

class RatingSummaryViewRenderer {
  class func render(_ control: RatingSummaryView, style: Style) {
    if let font = style.font {
      control.textFont = font
      control.textSize = font.pointSize
    }
    if let colors = style.colors, colors.count == 6 {
      control.noValueColor = colors[0]
      control.horribleColor = colors[1]
      control.badColor = colors[2]
      control.normalColor = colors[3]
      control.goodColor = colors[4]
      control.excellentColor = colors[5]
    }
    if let images = style.images, images.count == 6 {
      control.noValueImage = UIImage(named: images[0])
      control.horribleImage = UIImage(named: images[1])
      control.badImage = UIImage(named: images[2])
      control.normalImage = UIImage(named: images[3])
      control.goodImage = UIImage(named: images[4])
      control.excellentImage = UIImage(named: images[5])
    }
  }
}

