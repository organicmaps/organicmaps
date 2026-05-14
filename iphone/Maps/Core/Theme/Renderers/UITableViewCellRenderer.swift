extension UITableViewCell {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      setStyle(.tableViewCell)
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UITableViewCellRenderer.render(self, style: style)
    }
  }
}

class UITableViewCellRenderer {
  class func render(_ control: UITableViewCell, style: Style) {
    if let label = control.textLabel {
      if let fontStyle = style.fontStyle {
        label.font = fontStyle.font
        label.adjustsFontForContentSizeCategory = fontStyle.isDynamic
      }
      if let fontColor = style.fontColor {
        label.textColor = fontColor
      }
    }
    if let detailedLabel = control.detailTextLabel {
      if let fontStyle = style.fontDetailedStyle {
        detailedLabel.font = fontStyle.font
        detailedLabel.adjustsFontForContentSizeCategory = fontStyle.isDynamic
      }
      if let fontColorDetailed = style.fontColorDetailed {
        detailedLabel.textColor = fontColorDetailed
      }
    }
    if let fontColorDetailed = style.fontColorDetailed {
      control.imageView?.tintColor = fontColorDetailed
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
      control.accessoryView?.tintColor = tintColor
    }
    if let backgroundColor = style.backgroundColor {
      control.backgroundColor = backgroundColor
      let bgView = UIView()
      bgView.backgroundColor = backgroundColor
      control.backgroundView = bgView
    }
    if let backgroundColorSelected = style.backgroundColorSelected {
      let selView = UIView()
      selView.backgroundColor = backgroundColorSelected
      control.selectedBackgroundView = selView
    }
  }
}
