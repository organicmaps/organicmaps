enum NavigationDashboardModalPresentationStep: Int, ModalPresentationStep {
  case expanded
  case regular
  case compact
  case hidden
}

final class NavigationDashboardModalPresentationStepStrategy: ModalPresentationStepStrategy {
  private enum Constants {
    static let iPadWidth: CGFloat = 350
    static let iPadLeadingOffset: CGFloat = 20
    static let compactHeightOffset: CGFloat = 300
    static let fullScreenHeightFactorPortrait: CGFloat = 0.1
    static let halfScreenHeightFactorPortrait: CGFloat = 0.55
    static let topInset: CGFloat = 8
  }

  typealias Step = NavigationDashboardModalPresentationStep

  static func == (lhs: NavigationDashboardModalPresentationStepStrategy, rhs: NavigationDashboardModalPresentationStepStrategy) -> Bool {
    lhs.regularHeigh == rhs.regularHeigh && lhs.compactHeight == rhs.compactHeight
  }

  var regularHeigh: CGFloat = .zero
  var compactHeight: CGFloat = .zero

  func upperTo(_ step: Step) -> Step {
    switch step {
    case .expanded:
      return .expanded
    case .regular:
      return .expanded
    case .compact:
      return .regular
    case .hidden:
      return .regular
    }
  }

  func lowerTo(_ step: Step) -> Step {
    switch step {
    case .expanded:
      return .regular
    case .regular:
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

    func iPadFrame() -> CGRect {
      frame.size.width = Constants.iPadWidth
      frame.origin.x = Constants.iPadLeadingOffset
      switch step {
      case .expanded:
        frame.origin.y = safeAreaInsets.top + Constants.topInset
      case .regular:
        if regularHeigh != 0 {
          frame.origin.y = max(containerSize.height - regularHeigh, safeAreaInsets.top + Constants.topInset)
        } else {
          frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
        }
      case .compact:
        if compactHeight != 0 {
          frame.origin.y = containerSize.height - compactHeight
        } else {
          frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
        }
      case .hidden:
        frame.origin.y = containerSize.height
      }
      return frame
    }

    func iPhoneFrame() -> CGRect {
      let isPortraitOrientation = traitCollection.verticalSizeClass == .regular
      if isPortraitOrientation {
        switch step {
        case .expanded:
          frame.origin.y = safeAreaInsets.top + Constants.topInset
        case .regular:
          if regularHeigh != 0 {
            frame.origin.y = max(containerSize.height - regularHeigh, safeAreaInsets.top + Constants.topInset)
          } else {
            frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
          }
        case .compact:
          if compactHeight != 0 {
            frame.origin.y = containerSize.height - compactHeight
          } else {
            frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
          }
        case .hidden:
          frame.origin.y = containerSize.height
        }
      } else {
        frame.size.width = Constants.iPadWidth
        frame.origin.x = safeAreaInsets.left
        switch step {
        case .expanded, .regular:
          frame.origin.y = Constants.topInset
        case .compact:
          frame.origin.y = containerSize.height - (compactHeight != 0 ? compactHeight : Constants.compactHeightOffset)
        case .hidden:
          frame.origin.y = containerSize.height
        }
      }
      return frame
    }

    return isIPad ? iPadFrame() : iPhoneFrame()
  }
}
