extension TabView {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      setStyle(.tabView)
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      TabViewRenderer.render(self, style: style)
    }
  }
}

class TabViewRenderer {
  class func render(_ control: TabView, style: Style) {
    if let backgroundColor = style.backgroundColor {
      control.backgroundColor = backgroundColor
    }
    if let barTintColor = style.barTintColor {
      control.barTintColor = barTintColor
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
    if let font = style.font, let fontColor = style.fontColorHighlighted {
      control.selectedHeaderTextAttributes = [.foregroundColor: fontColor,
                                      .font: font]
    }
    if let font = style.font, let fontColor = style.fontColor {
      control.deselectedHeaderTextAttributes = [.foregroundColor: fontColor,
                                      .font: font]
    }
  }
}
