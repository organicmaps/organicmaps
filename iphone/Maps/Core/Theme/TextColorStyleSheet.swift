enum TextColorStyleSheet: String, CaseIterable {
  case whitePrimary = "whitePrimaryText"
  case blackSecondary = "blackSecondaryText"
  case blackPrimary = "blackPrimaryText"
  case linkBlue = "linkBlueText"
  case linkBlueHighlighted = "linkBlueHighlightedText"
  case white = "whiteText"
  case blackHint = "blackHintText"
  case green = "greenText"
  case red = "redText"
  case buttonRed = "buttonRedText"
}

extension TextColorStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    let color: UIColor = {
      switch self {
      case .whitePrimary: return colors.whitePrimaryText
      case .blackSecondary: return colors.blackSecondaryText
      case .blackPrimary: return colors.blackPrimaryText
      case .linkBlue: return colors.linkBlue
      case .linkBlueHighlighted: return colors.linkBlueHighlighted
      case .white: return colors.white
      case .blackHint: return colors.blackHintText
      case .green: return colors.ratingGreen
      case .red: return colors.red
      case .buttonRed: return colors.buttonRed
      }
    }()
    return .add { $0.fontColor = color }
  }
}
