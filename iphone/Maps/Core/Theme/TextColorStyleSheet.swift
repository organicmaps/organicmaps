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
  var styleResolver: Theme.StyleResolver {
    let color: UIColor = {
      switch self {
      case .whitePrimary: .whitePrimaryText
      case .blackSecondary: .blackSecondaryText
      case .blackPrimary: .blackPrimaryText
      case .linkBlue: .linkBlue
      case .linkBlueHighlighted: .linkBlueHighlighted
      case .white: .whitePrimary
      case .blackHint: .blackHintText
      case .green: .ratingGreen
      case .red: .redPrimary
      case .buttonRed: .buttonRed
      }
    }()
    return .add { $0.fontColor = color }
  }
}
