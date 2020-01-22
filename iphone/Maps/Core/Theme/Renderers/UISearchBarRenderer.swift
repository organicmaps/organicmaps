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
    if let backgroundColor = style.backgroundColor {
      control.searchTextField.backgroundColor = backgroundColor
    }
    if let barTintColor = style.barTintColor {
      control.barTintColor = barTintColor
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
      if let image = control.searchTextField.leftView as? UIImageView {
        image.tintColor = tintColor
      }
    }
    if let font = style.font {
      control.searchTextField.font = font
    }
    if let fontColor = style.fontColor {
      control.searchTextField.textColor = fontColor
    }
  }
}
