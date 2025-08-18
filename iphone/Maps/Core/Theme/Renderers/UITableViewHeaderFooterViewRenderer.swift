extension UITableViewHeaderFooterView {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      setStyle(.tableViewHeaderFooterView)
    }
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
    if let fontColor = style.fontColor {
      control.textLabel?.textColor = fontColor
    }
  }
}
