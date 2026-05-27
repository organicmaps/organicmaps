enum NavigationDashboardModalPresentationStep: Int, CaseIterable, ModalPresentationStep {
  case expanded
  case regular
  case halfScreen
  case compact
  case estimates
  case hidden
}

final class NavigationDashboardModalPresentationStepStrategy: ModalPresentationStepStrategy {
  private enum Constants {
    static let iPadWidth: CGFloat = 350
    static let iPadLeadingOffset: CGFloat = 20
    static let compactHeightOffset: CGFloat = 300
    static let halfScreenHeightFactorPortrait: CGFloat = 0.55
    static let halfScreenOriginFactorPortrait: CGFloat = 0.5
    static let topInset: CGFloat = 8
  }

  static let halfScreenActivationHeightFactor: CGFloat = 2.0 / 3.0

  typealias Step = NavigationDashboardModalPresentationStep

  static func == (lhs: NavigationDashboardModalPresentationStepStrategy, rhs: NavigationDashboardModalPresentationStepStrategy) -> Bool {
    lhs.regularHeight == rhs.regularHeight &&
      lhs.compactBaseHeight == rhs.compactBaseHeight &&
      lhs.estimatesHeight == rhs.estimatesHeight &&
      lhs.compactDiscoverabilityOffset == rhs.compactDiscoverabilityOffset &&
      lhs.shouldUseCompactDiscoverabilityOffset == rhs.shouldUseCompactDiscoverabilityOffset &&
      lhs.shouldShowHalfScreenStep == rhs.shouldShowHalfScreenStep &&
      lhs.shouldShowEstimatesStep == rhs.shouldShowEstimatesStep
  }

  var regularHeight: CGFloat = .zero
  var compactBaseHeight: CGFloat = .zero
  var estimatesHeight: CGFloat = .zero
  var compactDiscoverabilityOffset: CGFloat = .zero
  var shouldUseCompactDiscoverabilityOffset = true
  var shouldShowHalfScreenStep = false
  var shouldShowEstimatesStep = false

  var compactHeight: CGFloat {
    compactBaseHeight + (shouldUseCompactDiscoverabilityOffset ? compactDiscoverabilityOffset : 0)
  }

  var steps: [Step] {
    var availableSteps: [Step] = [.expanded, .regular]
    if shouldShowHalfScreenStep {
      availableSteps.append(.halfScreen)
    }
    availableSteps.append(.compact)
    if shouldShowEstimatesStep {
      availableSteps.append(.estimates)
    }
    availableSteps.append(.hidden)
    return availableSteps
  }

  func upperTo(_ step: Step) -> Step {
    switch step {
    case .expanded:
      return .expanded
    case .regular:
      return .expanded
    case .halfScreen:
      return .regular
    case .compact:
      return shouldShowHalfScreenStep ? .halfScreen : .regular
    case .estimates:
      return .compact
    case .hidden:
      return shouldShowEstimatesStep ? .estimates : .compact
    }
  }

  func lowerTo(_ step: Step) -> Step {
    switch step {
    case .expanded:
      return .regular
    case .regular:
      return shouldShowHalfScreenStep ? .halfScreen : .compact
    case .halfScreen:
      return .compact
    case .compact:
      return shouldShowEstimatesStep ? .estimates : .compact
    case .estimates:
      return .estimates
    case .hidden:
      return .hidden
    }
  }

  func resolvedStep(_ step: Step) -> Step {
    switch step {
    case .halfScreen where !shouldShowHalfScreenStep:
      return .regular
    case .estimates where !shouldShowEstimatesStep:
      return .compact
    default:
      return step
    }
  }

  func frame(_ step: Step, for _: UIView, in containerViewController: UIViewController) -> CGRect {
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
        if regularHeight != 0 {
          frame.origin.y = max(containerSize.height - regularHeight, safeAreaInsets.top + Constants.topInset)
        } else {
          frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
        }
      case .halfScreen:
        frame.origin.y = max(containerSize.height * Constants.halfScreenOriginFactorPortrait, safeAreaInsets.top + Constants.topInset)
      case .compact:
        if compactHeight != 0 {
          frame.origin.y = containerSize.height - compactHeight
        } else {
          frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
        }
      case .estimates:
        if estimatesHeight != 0 {
          frame.origin.y = containerSize.height - estimatesHeight
        } else {
          frame.origin.y = compactHeight != 0
            ? containerSize.height - compactHeight
            : containerSize.height * Constants.halfScreenHeightFactorPortrait
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
          if regularHeight != 0 {
            frame.origin.y = max(containerSize.height - regularHeight, safeAreaInsets.top + Constants.topInset)
          } else {
            frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
          }
        case .halfScreen:
          frame.origin.y = max(containerSize.height * Constants.halfScreenOriginFactorPortrait, safeAreaInsets.top + Constants.topInset)
        case .compact:
          if compactHeight != 0 {
            frame.origin.y = containerSize.height - compactHeight
          } else {
            frame.origin.y = containerSize.height * Constants.halfScreenHeightFactorPortrait
          }
        case .estimates:
          if estimatesHeight != 0 {
            frame.origin.y = containerSize.height - estimatesHeight
          } else {
            frame.origin.y = compactHeight != 0
              ? containerSize.height - compactHeight
              : containerSize.height * Constants.halfScreenHeightFactorPortrait
          }
        case .hidden:
          frame.origin.y = containerSize.height
        }
      } else {
        frame.size.width = Constants.iPadWidth
        frame.origin.x = safeAreaInsets.left
        switch step {
        case .expanded, .regular, .halfScreen:
          frame.origin.y = Constants.topInset
        case .compact:
          frame.origin.y = containerSize.height - (compactHeight != 0 ? compactHeight : Constants.compactHeightOffset)
        case .estimates:
          let height = estimatesHeight != 0 ? estimatesHeight : compactHeight
          frame.origin.y = containerSize.height - (height != 0 ? height : Constants.compactHeightOffset)
        case .hidden:
          frame.origin.y = containerSize.height
        }
      }
      return frame
    }

    return isIPad ? iPadFrame() : iPhoneFrame()
  }
}
