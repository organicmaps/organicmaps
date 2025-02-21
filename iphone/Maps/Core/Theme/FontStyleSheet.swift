enum FontStyleSheet: String, CaseIterable {
  case regular9
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

  case light10
  case light12
  case light16
  case light17

  case bold12
  case bold14
  case bold16
  case bold17
  case bold18
  case bold20
  case bold22
  case bold24
  case bold28
  case bold34
  case bold36
  case bold48

  case heavy17
  case heavy20
  case heavy32
  case heavy38

  case italic12
  case italic16

  case semibold12
  case semibold14
  case semibold15
  case semibold16
  case semibold18
}

extension FontStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    let font: UIFont = {
      switch self {
      case .regular9: return fonts.regular9
      case .regular10: return fonts.regular10
      case .regular11: return fonts.regular11
      case .regular12: return fonts.regular12
      case .regular13: return fonts.regular13
      case .regular14: return fonts.regular14
      case .regular15: return fonts.regular15
      case .regular16: return fonts.regular16
      case .regular17: return fonts.regular17
      case .regular18: return fonts.regular18
      case .regular20: return fonts.regular20
      case .regular24: return fonts.regular24
      case .regular32: return fonts.regular32
      case .regular52: return fonts.regular52

      case .medium9: return fonts.medium9
      case .medium10: return fonts.medium10
      case .medium12: return fonts.medium12
      case .medium13: return fonts.medium13
      case .medium14: return fonts.medium14
      case .medium16: return fonts.medium16
      case .medium17: return fonts.medium17
      case .medium18: return fonts.medium18
      case .medium20: return fonts.medium20
      case .medium24: return fonts.medium24
      case .medium28: return fonts.medium28
      case .medium36: return fonts.medium36
      case .medium40: return fonts.medium40
      case .medium44: return fonts.medium44

      case .light10: return fonts.light10
      case .light12: return fonts.light12
      case .light16: return fonts.light16
      case .light17: return fonts.light17

      case .bold12: return fonts.bold12
      case .bold14: return fonts.bold14
      case .bold16: return fonts.bold16
      case .bold17: return fonts.bold17
      case .bold18: return fonts.bold18
      case .bold20: return fonts.bold20
      case .bold22: return fonts.bold22
      case .bold24: return fonts.bold24
      case .bold28: return fonts.bold28
      case .bold34: return fonts.bold34
      case .bold36: return fonts.bold36
      case .bold48: return fonts.bold48

      case .heavy17: return fonts.heavy17
      case .heavy20: return fonts.heavy20
      case .heavy32: return fonts.heavy32
      case .heavy38: return fonts.heavy38

      case .italic12: return fonts.italic12
      case .italic16: return fonts.italic16

      case .semibold12: return fonts.semibold12
      case .semibold14: return fonts.semibold14
      case .semibold15: return fonts.semibold15
      case .semibold16: return fonts.semibold16
      case .semibold18: return fonts.semibold18
      }
    }()
    return .add { s in s.font = font }
  }
}
