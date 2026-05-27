import Foundation

extension UISearchBar {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      setStyle(.searchBar)
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UISearchBarRenderer.render(self, style: style)
    }
  }

  @objc override func sw_didMoveToWindow() {
    guard MapsAppDelegate.theApp().window === window else {
      sw_didMoveToWindow()
      return
    }
    applyTheme()
    isStyleApplied = true
    sw_didMoveToWindow()
  }
}

class UISearchBarRenderer: UIViewRenderer {
  class func render(_ control: UISearchBar, style: Style) {
    if #available(iOS 26.0, *) {
      // The search bar customizations breaks the new `LiquidGlass` style that was introduced in iOS 26.0. The native iOS style will be used.
      if let backgroundColor = style.backgroundColor {
        control.searchTextField.backgroundColor = backgroundColor
      }
      return
    }
    super.render(control, style: style)
    let searchTextField = control.searchTextField
    // Default search bar implementation adds the grey transparent image for background. This code removes it and updates the corner radius. This is not working on iPad designed for mac.
    if ProcessInfo.processInfo.isiOSAppOnMac {
    } else {
      control.setSearchFieldBackgroundImage(UIImage(), for: .normal)
    }
    searchTextField.layer.setCornerRadius(.buttonDefault)
    searchTextField.layer.masksToBounds = true
    // Placeholder color
    if let placeholder = searchTextField.placeholder {
      searchTextField.attributedPlaceholder = NSAttributedString(string: placeholder, attributes: [.foregroundColor: UIColor.gray])
    }
    if let backgroundColor = style.backgroundColor {
      searchTextField.backgroundColor = backgroundColor
    }
    if let font = style.font {
      searchTextField.font = font
    }
    if let fontColor = style.fontColor {
      searchTextField.textColor = fontColor
    }
    if let tintColor = style.tintColor {
      searchTextField.leftView?.tintColor = tintColor
      // Placeholder indicator color
      searchTextField.tintColor = tintColor
      // Clear button image
      let clearButtonImage = UIImage(named: "ic_clear")?.withRenderingMode(.alwaysTemplate).withTintColor(tintColor)
      control.setImage(clearButtonImage, for: .clear, state: .normal)
    }
    if let barTintColor = style.barTintColor {
      let traits = control.window?.traitCollection ?? control.traitCollection
      let position = control.delegate?.position?(for: control) ?? control.barPosition
      control.setBackgroundImage(barTintColor.getImage(traits), for: position, barMetrics: .defaultPrompt)
      control.setBackgroundImage(barTintColor.getImage(traits), for: position, barMetrics: .default)
      control.backgroundColor = barTintColor
    }
    if let fontColorDetailed = style.fontColorDetailed {
      // Cancel button color
      control.tintColor = fontColorDetailed
    }
  }
}
