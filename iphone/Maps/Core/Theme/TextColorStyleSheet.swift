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
  func styleResolverFor(fonts _: IFonts) -> Theme.StyleResolver {
    let color: UIColor = {
      switch self {
      case .whitePrimary: return UIColor(named: "whitePrimaryText")!
      case .blackSecondary: return UIColor(named: "blackSecondaryText")!
      case .blackPrimary: return UIColor(named: "blackPrimaryText")!
      case .linkBlue: return UIColor(named: "linkBlue")!
      case .linkBlueHighlighted: return UIColor(named: "linkBlueHighlighted")!
      case .white: return UIColor(named: "whitePrimary")!
      case .blackHint: return UIColor(named: "blackHintText")!
      case .green: return UIColor(named: "ratingGreen")!
      case .red: return UIColor(named: "redPrimary")!
      case .buttonRed: return UIColor(named: "buttonRed")!
      }
    }()
    return .add { $0.fontColor = color }
  }
}
