extension UITableViewHeaderFooterView {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UITableViewHeaderFooterViewRenderer.render(self, style: style)
    }
  }
}

class UITableViewHeaderFooterViewRenderer {
  class func render(_ control: UITableViewHeaderFooterView, style: Style) {
    if let backgroundColor = style.backgroundColor {
      control.backgroundView = UIImageView(image: backgroundColor.getImage())
    }
    if let font = style.font {
      control.textLabel?.font = font
    }
    if let fontColor = style.fontColor {
      control.textLabel?.textColor = fontColor
    }
  }
}
