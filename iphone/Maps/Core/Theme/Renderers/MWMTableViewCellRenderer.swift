import Foundation
extension MWMTableViewCell {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "MWMTableViewCell"
    }
    for style in StyleManager.instance().getStyle(styleName) {
      MWMTableViewCellRenderer.render(self, style: style)
    }
  }
}

class MWMTableViewCellRenderer {
  class func render(_ control: MWMTableViewCell, style: Style) {
    if let label = control.textLabel {
      if let font = style.font {
        label.font = font
      }
      if let fontColor = style.fontColor {
        label.textColor = fontColor
      }
    }
    if let detailedLabel = control.detailTextLabel {
      if let fontDetailed = style.fontDetailed {
        detailedLabel.font = fontDetailed
      }
      if let fontColorDetailed = style.fontColorDetailed {
        detailedLabel.textColor = fontColorDetailed
      }
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
      control.accessoryView?.tintColor = tintColor
    }
    if let backgroundColor = style.backgroundColor {
      control.backgroundColor = backgroundColor
      control.backgroundView = UIImageView(image: backgroundColor.getImage())
    }
    if let backgroundColorSelected = style.backgroundColorSelected {
      control.selectedBackgroundView = UIImageView(image: backgroundColorSelected.getImage())
    }
  }
}
