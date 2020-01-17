import Foundation
extension UISearchBar {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "SearchBar"
    }
    for style in StyleManager.instance().getStyle(styleName) {
      UIViewRenderer.render(self, style: style)
      UISearchBarRenderer.render(self, style: style)
    }
  }
}

class UISearchBarRenderer {
  class func render(_ control: UISearchBar, style: Style) {
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
