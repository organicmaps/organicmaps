import UIKit

class UIBarButtonItemRenderer {
  class func render(_ control: UIBarButtonItem, style: Style) {
    var normalAttributes = [NSAttributedString.Key: Any]()
    var disableAttributes = [NSAttributedString.Key: Any]()
    var highlightedAttributes = [NSAttributedString.Key: Any]()

    if let backgroundImage = style.backgroundImage {
      control.setBackgroundImage(backgroundImage, for: .normal, barMetrics: .default)
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
    if let fontStyle = style.fontStyle {
      normalAttributes[NSAttributedString.Key.font] = fontStyle.font
      disableAttributes[NSAttributedString.Key.font] = fontStyle.font
      highlightedAttributes[NSAttributedString.Key.font] = fontStyle.font
    }
    if let fontColor = style.fontColor {
      normalAttributes[NSAttributedString.Key.foregroundColor] = fontColor
    }
    if let fontColorDisabled = style.fontColorDisabled {
      disableAttributes[NSAttributedString.Key.foregroundColor] = fontColorDisabled
    }
    if let fontColorHighlighted = style.fontColorHighlighted {
      highlightedAttributes[NSAttributedString.Key.foregroundColor] = fontColorHighlighted
    }

    control.setTitleTextAttributes(normalAttributes, for: .normal)
    control.setTitleTextAttributes(disableAttributes, for: .disabled)
    control.setTitleTextAttributes(highlightedAttributes, for: .highlighted)
  }
}
