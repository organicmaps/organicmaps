extension UITableView {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "TableView"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UITableViewRenderer.render(self, style: style)
    }
  }
}

class UITableViewRenderer: UIViewRenderer {
  class func render(_ control: UITableView, style: Style) {
    super.render(control, style: style)
    if let backgroundColor = style.backgroundColor {
      control.backgroundView = UIImageView(image: backgroundColor.getImage())
    }
    if let separatorColor = style.separatorColor {
      control.separatorColor = separatorColor
    }
    UIViewRenderer.renderBorder(control, style: style)
    UIViewRenderer.renderShadow(control, style: style)
  }
}
