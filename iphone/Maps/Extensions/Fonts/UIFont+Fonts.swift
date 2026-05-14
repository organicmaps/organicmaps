extension UIFont {
  @objc static var regular9: UIFont { systemFont(ofSize: 9, weight: .regular) }
  @objc static var regular10: UIFont { systemFont(ofSize: 10, weight: .regular) }
  @objc static var regular11: UIFont { systemFont(ofSize: 11, weight: .regular) }
  @objc static var regular12: UIFont { systemFont(ofSize: 12, weight: .regular) }
  @objc static var regular13: UIFont { systemFont(ofSize: 13, weight: .regular) }
  @objc static var regular14: UIFont { systemFont(ofSize: 14, weight: .regular) }
  @objc static var regular15: UIFont { systemFont(ofSize: 15, weight: .regular) }
  @objc static var regular16: UIFont { systemFont(ofSize: 16, weight: .regular) }
  @objc static var regular17: UIFont { systemFont(ofSize: 17, weight: .regular) }
  @objc static var regular18: UIFont { systemFont(ofSize: 18, weight: .regular) }
  @objc static var regular20: UIFont { systemFont(ofSize: 20, weight: .regular) }
  @objc static var regular24: UIFont { systemFont(ofSize: 24, weight: .regular) }
  @objc static var regular32: UIFont { systemFont(ofSize: 32, weight: .regular) }
  @objc static var regular52: UIFont { systemFont(ofSize: 52, weight: .regular) }

  @objc static var medium9: UIFont { systemFont(ofSize: 9, weight: .medium) }
  @objc static var medium10: UIFont { systemFont(ofSize: 10, weight: .medium) }
  @objc static var medium12: UIFont { systemFont(ofSize: 12, weight: .medium) }
  @objc static var medium13: UIFont { systemFont(ofSize: 13, weight: .medium) }
  @objc static var medium14: UIFont { systemFont(ofSize: 14, weight: .medium) }
  @objc static var medium15: UIFont { systemFont(ofSize: 15, weight: .medium) }
  @objc static var medium16: UIFont { systemFont(ofSize: 16, weight: .medium) }
  @objc static var medium17: UIFont { systemFont(ofSize: 17, weight: .medium) }
  @objc static var medium18: UIFont { systemFont(ofSize: 18, weight: .medium) }
  @objc static var medium20: UIFont { systemFont(ofSize: 20, weight: .medium) }
  @objc static var medium24: UIFont { systemFont(ofSize: 24, weight: .medium) }
  @objc static var medium28: UIFont { systemFont(ofSize: 28, weight: .medium) }
  @objc static var medium36: UIFont { systemFont(ofSize: 36, weight: .medium) }
  @objc static var medium40: UIFont { systemFont(ofSize: 40, weight: .medium) }
  @objc static var medium44: UIFont { systemFont(ofSize: 44, weight: .medium) }

  @objc static var light12: UIFont { systemFont(ofSize: 12, weight: .light) }

  @objc static var bold12: UIFont { systemFont(ofSize: 12, weight: .bold) }
  @objc static var bold14: UIFont { systemFont(ofSize: 14, weight: .bold) }
  @objc static var bold16: UIFont { systemFont(ofSize: 16, weight: .bold) }
  @objc static var bold17: UIFont { systemFont(ofSize: 17, weight: .bold) }
  @objc static var bold18: UIFont { systemFont(ofSize: 18, weight: .bold) }
  @objc static var bold20: UIFont { systemFont(ofSize: 20, weight: .bold) }
  @objc static var bold22: UIFont { systemFont(ofSize: 22, weight: .bold) }
  @objc static var bold24: UIFont { systemFont(ofSize: 24, weight: .bold) }
  @objc static var bold28: UIFont { systemFont(ofSize: 28, weight: .bold) }
  @objc static var bold36: UIFont { systemFont(ofSize: 36, weight: .bold) }

  @objc static var semibold14: UIFont { systemFont(ofSize: 14, weight: .semibold) }
  @objc static var semibold16: UIFont { systemFont(ofSize: 16, weight: .semibold) }
  @objc static var semibold17: UIFont { systemFont(ofSize: 17, weight: .semibold) }
  @objc static var semibold18: UIFont { systemFont(ofSize: 18, weight: .semibold) }

  @objc static var emojiMedium13: UIFont { emojiFont(ofSize: 13, weight: .medium) }
  @objc static var emojiRegular14: UIFont { emojiFont(ofSize: 14, weight: .regular) }

  static var header: UIFont { .preferredFont(forTextStyle: .headline) }
}

extension UIFont {
  static func emojiFont(ofSize fontSize: CGFloat, weight: UIFont.Weight) -> UIFont {
    guard let emojiFont = UIFont(name: "OrganicMapsEmoji", size: fontSize) else {
      return systemFont(ofSize: fontSize, weight: weight)
    }
    let fallbackFont = systemFont(ofSize: fontSize, weight: weight)
    let cascadeDescriptor = emojiFont.fontDescriptor.addingAttributes([
      .cascadeList: [fallbackFont.fontDescriptor],
    ])
    return UIFont(descriptor: cascadeDescriptor, size: fontSize)
  }
}
