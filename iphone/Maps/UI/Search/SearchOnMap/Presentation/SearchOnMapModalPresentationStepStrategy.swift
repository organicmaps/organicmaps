enum SearchOnMapModalPresentationStep: Int, CaseIterable, ModalPresentationStep {
  case expanded
  case halfScreen
  case compact
  case hidden
}

struct SearchOnMapModalPresentationStepStrategy: ModalPresentationStepStrategy {
  private enum Constants {
    static let iPadWidth: CGFloat = 350
    static let compactHeightOffset: CGFloat = 120
    static let halfScreenHeightFactorPortrait: CGFloat = 0.55
    static let topInset: CGFloat = 8
  }

  typealias Step = SearchOnMapModalPresentationStep

  func upperTo(_ step: Step) -> Step {
    switch step {
    case .expanded:
      return .expanded
    case .halfScreen:
      return .expanded
    case .compact:
      return .halfScreen
    case .hidden:
      return .compact
    }
  }

  func lowerTo(_ step: Step) -> Step {
    switch step {
    case .expanded:
      return .halfScreen
    case .halfScreen:
      return .compact
    case .compact:
      return .compact
    case .hidden:
      return .hidden
    }
  }

  var first: Step {
    .expanded
  }

  var last: Step {
    .compact
  }

  func frame(_ step: Step, for presentedView: UIView, in containerViewController: UIViewController) -> CGRect {
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
      switch step {
      case .hidden:
        frame.origin.x = -Constants.iPadWidth
      default:
        frame.origin.x = .zero
      }
      return frame
    }

    let isPortraitOrientation = traitCollection.verticalSizeClass == .regular
    if isPortraitOrientation {
      switch step {
      case .expanded:
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
      switch step {
      case .expanded:
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

