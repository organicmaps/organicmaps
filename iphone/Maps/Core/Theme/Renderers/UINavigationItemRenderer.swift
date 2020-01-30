extension UINavigationItem {
  @objc func applyTheme() {
    if styleName.isEmpty {
      styleName = "NavigationBarItem"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty {
      UINavigationItemRenderer.render(self, style: style)
    }
  }
}
class UINavigationItemRenderer {
  class func render(_ control: UINavigationItem, style: Style) {
    if let item = control.backBarButtonItem {
      UIBarButtonItemRenderer.render(item, style: style)
    }
    if let item = control.leftBarButtonItem {
      UIBarButtonItemRenderer.render(item, style: style)
    }
    if let item = control.rightBarButtonItem {
      UIBarButtonItemRenderer.render(item, style: style)
    }
  }
}
