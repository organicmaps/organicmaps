extension UIFont {
  @objc
  var dynamic: UIFont {
    UIFontMetrics(forTextStyle: bestStyle).scaledFont(for: self)
  }

  func dynamic(maxSize: CGFloat) -> UIFont {
    UIFontMetrics(forTextStyle: bestStyle).scaledFont(for: self, maximumPointSize: maxSize)
  }

  private var bestStyle: UIFont.TextStyle {
    switch pointSize {
    case ...11:
      return .caption2
    case 12:
      return .caption1
    case 13 ... 14:
      return .footnote
    case 15:
      return .subheadline
    case 16:
      return .callout
    case 17:
      return .body
    case 18 ... 20:
      return .title3
    case 21 ... 22:
      return .title2
    case 23 ... 28:
      return .title1
    default:
      return .largeTitle
    }
  }
}
