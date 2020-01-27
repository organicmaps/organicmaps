import Foundation
extension UISearchBar {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "SearchBar"
    }
    for style in StyleManager.shared.getStyle(styleName) {
      UISearchBarRenderer.render(self, style: style)
    }
  }
}

class UISearchBarRenderer: UIViewRenderer {
  class func render(_ control: UISearchBar, style: Style) {
    super.render(control, style: style)
    if let barTintColor = style.barTintColor {
      control.barTintColor = barTintColor
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
  }

  //fix for iOS 12 and below
  class func setAppearance() {
    for style in StyleManager.shared.getStyle("SearchBar") {
      if let backgroundColor = style.backgroundColor {
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).backgroundColor = backgroundColor
      }
      if let tintColor = style.tintColor {
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).leftView?.tintColor = tintColor
      }
      if let font = style.font {
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).font = font
      }
      if let fontColor = style.fontColor {
        UITextField.appearance(whenContainedInInstancesOf: [UISearchBar.self]).textColor = fontColor
      }
    }
  }
}
