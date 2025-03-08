enum ModalScreenPresentationStep {
  case fullScreen
  case halfScreen
  case compact
  case hidden
}

extension ModalScreenPresentationStep {
  private enum Constants {
    static let iPadWidth: CGFloat = 350
    static let compactHeightOffset: CGFloat = 120
    static let fullScreenHeightFactorPortrait: CGFloat = 0.1
    static let halfScreenHeightFactorPortrait: CGFloat = 0.55
    static let landscapeTopInset: CGFloat = 10
  }

  var upper: ModalScreenPresentationStep {
    switch self {
    case .fullScreen:
      return .fullScreen
    case .halfScreen:
      return .fullScreen
    case .compact:
      return .halfScreen
    case .hidden:
      return .compact
    }
  }

  var lower: ModalScreenPresentationStep {
    switch self {
    case .fullScreen:
      return .halfScreen
    case .halfScreen:
      return .compact
    case .compact:
      return .compact
    case .hidden:
      return .hidden
    }
  }

  var first: ModalScreenPresentationStep {
    .fullScreen
  }

  var last: ModalScreenPresentationStep {
    .compact
  }

  func frame() -> CGRect {
    let isIPad = UIDevice.current.userInterfaceIdiom == .pad
    let containerSize = UIScreen.main.bounds.size
    let safeAreaInsets = UIApplication.shared.keyWindow?.safeAreaInsets ?? .zero
    let traitCollection = UIScreen.main.traitCollection
    var frame = CGRect(origin: .zero, size: containerSize)

    if isIPad {
      frame.size.width = Constants.iPadWidth
      switch self {
      case .hidden:
        frame.origin.x = -Constants.iPadWidth
      default:
        frame.origin.x = .zero
      }
      return frame
    }

    let isPortraitOrientation = traitCollection.verticalSizeClass == .regular
    if isPortraitOrientation {
      switch self {
      case .fullScreen:
        frame.origin.y = containerSize.height * Constants.fullScreenHeightFactorPortrait
      case .halfScreen:
        frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
      case .compact:
        frame.origin.y = containerSize.height - Constants.compactHeightOffset
      case .hidden:
        frame.origin.y = containerSize.height
      }
    } else {
      frame.size.width = Constants.iPadWidth
      frame.origin.x = safeAreaInsets.left
      switch self {
      case .fullScreen:
        frame.origin.y = Constants.landscapeTopInset
      case .halfScreen, .compact:
        frame.origin.y = containerSize.height - Constants.compactHeightOffset
      case .hidden:
        frame.origin.y = containerSize.height
      }
    }
    return frame
  }
}
