enum ModalPresentationStep: Int, CaseIterable {
  case fullScreen
  case halfScreen
  case compact
  case hidden
}

extension ModalPresentationStep {
  private enum Constants {
    static let iPadWidth: CGFloat = 350
    static let compactHeightOffset: CGFloat = 120
    static let halfScreenHeightFactorPortrait: CGFloat = 0.55
    static let topInset: CGFloat = 8
  }

  var upper: ModalPresentationStep {
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

  var lower: ModalPresentationStep {
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

  var first: ModalPresentationStep {
    .fullScreen
  }

  var last: ModalPresentationStep {
    .compact
  }

  func frame(for presentedView: UIView, in containerViewController: UIViewController) -> CGRect {
    let isIPad = UIDevice.current.userInterfaceIdiom == .pad
    var containerSize = containerViewController.view.bounds.size
    if containerSize == .zero {
      containerSize = UIScreen.main.bounds.size
    }
    let safeAreaInsets = containerViewController.view.safeAreaInsets
    let traitCollection = containerViewController.traitCollection
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
        frame.origin.y = safeAreaInsets.top + Constants.topInset
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
        frame.origin.y = Constants.topInset
      case .halfScreen, .compact:
        frame.origin.y = containerSize.height - Constants.compactHeightOffset
      case .hidden:
        frame.origin.y = containerSize.height
      }
    }
    return frame
  }
}
