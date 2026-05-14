struct FontStyle {
  enum DynamicBehavior {
    case fixed
    case dynamic(maxSize: CGFloat?)
  }

  private let baseFont: UIFont
  let dynamicBehavior: DynamicBehavior

  static func dynamic(_ font: UIFont, maxSize: CGFloat? = nil) -> FontStyle {
    FontStyle(baseFont: font, dynamicBehavior: .dynamic(maxSize: maxSize))
  }

  static func fixed(_ font: UIFont) -> FontStyle {
    FontStyle(baseFont: font, dynamicBehavior: .fixed)
  }

  var font: UIFont {
    switch dynamicBehavior {
    case .fixed:
      baseFont
    case .dynamic(let maxSize):
      if let maxSize {
        baseFont.dynamic(maxSize: maxSize)
      } else {
        baseFont.dynamic
      }
    }
  }

  var isDynamic: Bool {
    switch dynamicBehavior {
    case .fixed: false
    case .dynamic: true
    }
  }
}

enum FontStyleSheet: String, CaseIterable {
  case regular10
  case regular11
  case regular12
  case regular13
  case regular14
  case regular15
  case regular16
  case regular17
  case regular18
  case regular20
  case regular24
  case regular32
  case regular52

  case medium9
  case medium10
  case medium12
  case medium13
  case medium14
  case medium16
  case medium17
  case medium18
  case medium20
  case medium24
  case medium28
  case medium36
  case medium40
  case medium44

  case light12

  case bold12
  case bold14
  case bold16
  case bold17
  case bold18
  case bold20
  case bold22
  case bold24
  case bold28
  case bold36

  case semibold14
  case semibold16
  case semibold17
  case semibold18

  case emojiMedium13
}

extension FontStyleSheet: IStyleSheet {
  var font: UIFont {
    switch self {
    case .regular10: .regular10
    case .regular11: .regular11
    case .regular12: .regular12
    case .regular13: .regular13
    case .regular14: .regular14
    case .regular15: .regular15
    case .regular16: .regular16
    case .regular17: .regular17
    case .regular18: .regular18
    case .regular20: .regular20
    case .regular24: .regular24
    case .regular32: .regular32
    case .regular52: .regular52
    case .medium9: .medium9
    case .medium10: .medium10
    case .medium12: .medium12
    case .medium13: .medium13
    case .medium14: .medium14
    case .medium16: .medium16
    case .medium17: .medium17
    case .medium18: .medium18
    case .medium20: .medium20
    case .medium24: .medium24
    case .medium28: .medium28
    case .medium36: .medium36
    case .medium40: .medium40
    case .medium44: .medium44
    case .light12: .light12
    case .bold12: .bold12
    case .bold14: .bold14
    case .bold16: .bold16
    case .bold17: .bold17
    case .bold18: .bold18
    case .bold20: .bold20
    case .bold22: .bold22
    case .bold24: .bold24
    case .bold28: .bold28
    case .bold36: .bold36
    case .semibold14: .semibold14
    case .semibold16: .semibold16
    case .semibold17: .semibold17
    case .semibold18: .semibold18
    case .emojiMedium13: .emojiMedium13
    }
  }

  var styleResolver: Theme.StyleResolver {
    .add { s in s.fontStyle = .dynamic(font) }
  }
}
