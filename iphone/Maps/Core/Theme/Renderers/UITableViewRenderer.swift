extension UITableView {
  @objc override func applyTheme() {
    // Separators use UIKit's automatic leading inset and reach the cell's trailing edge app-wide.
    separatorInset = UIEdgeInsets(top: 0, left: UITableView.automaticDimension, bottom: 0, right: 0)
    if styleName.isEmpty {
      setStyle(.tableView)
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
      let bgView = UIView()
      bgView.backgroundColor = backgroundColor
      control.backgroundView = bgView
    }
    if let separatorColor = style.separatorColor {
      control.separatorColor = separatorColor
    }
    UIViewRenderer.renderBorder(control, style: style)
    UIViewRenderer.renderShadow(control, style: style)
  }
}
