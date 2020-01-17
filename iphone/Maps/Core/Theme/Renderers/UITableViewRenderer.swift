extension UITableView {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "TableView"
    }
    for style in StyleManager.instance().getStyle(styleName) {
      UIViewRenderer.render(self, style: style)
      UITableViewRenderer.render(self, style: style)
    }
  }
}

class UITableViewRenderer {
  class func render(_ control: UITableView, style: Style) {
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
