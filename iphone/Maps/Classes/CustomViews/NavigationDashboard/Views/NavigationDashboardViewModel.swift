enum NavigationDashboard {
  struct ViewModel: Equatable {
    let transportOptions: [MWMRouterType]
    let routePoints: RoutePoints
    let routerType: MWMRouterType
    let entity: MWMNavigationDashboardEntity
    let routingOptions: RoutingOptions
    let elevationInfo: ElevationInfo?
    let navigationInfo: NavigationInfo
    let estimates: NSAttributedString
    let dashboardState: MWMNavigationDashboardState
    let presentationStep: NavigationDashboardModalPresentationStep
    let progress: CGFloat
    let navigationSearchState: NavigationSearchState?
    let canSaveRouteAsTrack: Bool
    let errorMessage: String?
  }
  
  struct ElevationInfo: Equatable {
    let estimates: NSAttributedString
    let image: UIImage?
  }

  struct NavigationInfo: Equatable {
    let state: MWMNavigationInfoViewState
    let availableArea: CGRect
    let shouldUpdateToastView: Bool
  }
}

extension NavigationDashboard.ViewModel {
  static var initial: Self {
    NavigationDashboard.ViewModel(
      transportOptions: MWMRouterType.allCases,
      routePoints: .empty,
      routerType: MWMRouter.type(),
      entity: MWMNavigationDashboardEntity(),
      routingOptions: RoutingOptions(),
      elevationInfo: nil,
      navigationInfo: .hidden,
      estimates: NSAttributedString(),
      dashboardState: .hidden,
      presentationStep: .hidden,
      progress: 0,
      navigationSearchState: nil,
      canSaveRouteAsTrack: false,
      errorMessage: nil
    )
  }

  func copyWith(
    transportOptions: [MWMRouterType]? = nil,
    routePoints: NavigationDashboard.RoutePoints? = nil,
    routerType: MWMRouterType? = nil,
    entity: MWMNavigationDashboardEntity? = nil,
    routingOptions: RoutingOptions? = nil,
    elevationInfo: NavigationDashboard.ElevationInfo?? = nil,
    navigationInfo: NavigationDashboard.NavigationInfo? = nil,
    estimates: NSAttributedString? = nil,
    dashboardState: MWMNavigationDashboardState? = nil,
    presentationStep: NavigationDashboardModalPresentationStep? = nil,
    progress: CGFloat? = nil,
    navigationSearchState: NavigationSearchState? = nil,
    canSaveRouteAsTrack: Bool? = nil,
    errorMessage: String? = nil
  ) -> NavigationDashboard.ViewModel {
    return NavigationDashboard.ViewModel(
      transportOptions: transportOptions ?? self.transportOptions,
      routePoints: routePoints ?? self.routePoints,
      routerType: routerType ?? self.routerType,
      entity: entity ?? self.entity,
      routingOptions: routingOptions ?? self.routingOptions,
      elevationInfo: elevationInfo ?? self.elevationInfo,
      navigationInfo: navigationInfo ?? self.navigationInfo.copyWith(dashboardState: self.dashboardState),
      estimates: estimates ?? self.estimates,
      dashboardState: dashboardState ?? self.dashboardState,
      presentationStep: presentationStep ?? self.presentationStep,
      progress: progress ?? self.progress,
      navigationSearchState: navigationSearchState,
      canSaveRouteAsTrack: canSaveRouteAsTrack ?? self.canSaveRouteAsTrack,
      errorMessage: errorMessage ?? self.errorMessage
    )
  }

  var isBottomActionsMenuHidden: Bool { presentationStep == .hidden }

  var startButtonState: StartRouteButton.State {
    if routePoints.count < 2 ||
       routePoints.start == nil ||
       routePoints.finish == nil ||
       dashboardState == .error {
      return .disabled
    }
    if progress < 1 {
      return .loading
    }
    if routerType == .ruler ||
        routerType == .publicTransport {
      return .disabled
    }
    return .enabled
  }

  var estimatesState: EstimatesView.State {
    if dashboardState == .error, let errorMessage {
      return .error(errorMessage)
    }
    if progress < 1 {
      return .loading
    }
    return .estimates(estimates)
  }
}

extension NavigationDashboard.NavigationInfo {
  static let hidden = NavigationDashboard.NavigationInfo(
    state: .hidden,
    availableArea: MapViewController.shared()?.navigationInfoArea.areaFrame ?? .screenBounds,
    shouldUpdateToastView: false
  )

  func copyWith(
    dashboardState: MWMNavigationDashboardState? = nil,
    availableArea: CGRect? = nil,
    shouldUpdateToastView: Bool? = nil
  ) -> NavigationDashboard.NavigationInfo {
    return NavigationDashboard.NavigationInfo(
      state: dashboardState?.navigationInfo ?? self.state,
      availableArea: availableArea ?? self.availableArea,
      shouldUpdateToastView: shouldUpdateToastView ?? self.shouldUpdateToastView
    )
  }
}

private extension MWMNavigationDashboardState {
  var navigationInfo: MWMNavigationInfoViewState {
    switch self {
    case .navigation: return .navigation
    default: return .hidden
    }
  }
}

extension CGRect {
  static var screenBounds: CGRect {
    return UIScreen.main.bounds
  }
}

extension RoutingOptions {
  var enabledOptionsCount: Int {
    [avoidToll, avoidDirty, avoidFerry, avoidMotorway].filter { $0 }.count
  }
}
