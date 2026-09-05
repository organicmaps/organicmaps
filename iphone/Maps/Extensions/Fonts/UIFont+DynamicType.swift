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

@objc
final class MapFontScaleFactor: NSObject {
  @objc(valueForContentSizeCategory:)
  static func value(for contentSizeCategory: UIContentSizeCategory) -> Double {
    switch contentSizeCategory {
    case .extraSmall:
      return 0.8
    case .small:
      return 0.9
    case .medium, .large:
      return 1.0
    case .extraLarge:
      return 1.1
    case .extraExtraLarge:
      return 1.2
    case .extraExtraExtraLarge:
      return 1.3
    case .accessibilityMedium:
      return 1.4
    case .accessibilityLarge:
      return 1.5
    case .accessibilityExtraLarge:
      return 1.6
    case .accessibilityExtraExtraLarge:
      return 1.7
    case .accessibilityExtraExtraExtraLarge:
      return 1.8
    default:
      return 1.0
    }
  }
}
