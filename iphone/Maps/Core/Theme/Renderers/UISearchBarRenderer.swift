import Foundation
extension UISearchBar {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "SearchBar"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UISearchBarRenderer.render(self, style: style)
    }
  }

  @objc override func sw_didMoveToWindow() {
    guard MapsAppDelegate.theApp().window === window else {
      sw_didMoveToWindow();
      return
    }
    applyTheme()
    isStyleApplied = true
    sw_didMoveToWindow();
  }
}

class UISearchBarRenderer: UIViewRenderer {
  class func render(_ control: UISearchBar, style: Style) {
    super.render(control, style: style)
    var searchTextField: UITextField?
    if #available(iOS 13, *) {
      searchTextField = control.searchTextField
    }
    if let backgroundColor = style.backgroundColor {
      searchTextField?.backgroundColor = backgroundColor
    }
    if let barTintColor = style.barTintColor {
      let position = control.delegate?.position?(for: control) ?? control.barPosition
      control.setBackgroundImage(barTintColor.getImage(), for: position, barMetrics: .defaultPrompt)
      control.setBackgroundImage(barTintColor.getImage(), for: position, barMetrics: .default)
      control.backgroundColor = barTintColor
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
    if let font = style.font {
      searchTextField?.font = font
    }
    if let fontColor = style.fontColor {
      searchTextField?.textColor = fontColor
      searchTextField?.leftView?.tintColor = fontColor
      searchTextField?.tintColor = fontColor
    }
  }

  //fix for iOS 12 and below
  class func setAppearance() {
    for style in StyleManager.shared.getStyle("SearchBar") {
      if let backgroundColor = style.backgroundColor {
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).backgroundColor = backgroundColor
      }
      if let font = style.font {
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).font = font
      }
      if let fontColor = style.fontColor {
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).textColor = fontColor
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).leftView?.tintColor = fontColor
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).tintColor = fontColor
      }
    }
  }
}
