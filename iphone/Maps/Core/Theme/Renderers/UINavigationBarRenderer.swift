
extension UINavigationBar {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "NavigationBar"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UINavigationBarRenderer.render(self, style: style)
    }
  }
}

class UINavigationBarRenderer: UIViewRenderer {
  class func render(_ control: UINavigationBar, style: Style) {
    super.render(control, style: style)
    // there were some codes here that were forcing to show navigation bar on other storyboards
    control.isHidden = true
  }
}
