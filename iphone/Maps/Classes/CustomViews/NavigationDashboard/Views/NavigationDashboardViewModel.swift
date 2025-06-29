enum NavigationDashboard {
  struct ViewModel: Equatable {
    let transportOptions: [MWMRouterType]
    let routePoints: RoutePoints
    let routerType: MWMRouterType
    let entity: MWMNavigationDashboardEntity
    let elevationInfo: ElevationInfo?
    let navigationInfo: NavigationInfo
    let estimates: NSAttributedString
    let dashboardState: MWMNavigationDashboardState
    let presentationStep: NavigationDashboardModalPresentationStep
    let progress: CGFloat
    let navigationSearchState: NavigationSearchState?
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
  static let initial = NavigationDashboard.ViewModel(
    transportOptions: MWMRouterType.allCases,
    routePoints: .empty,
    routerType: .vehicle,
    entity: MWMNavigationDashboardEntity(),
    elevationInfo: nil,
    navigationInfo: .hidden,
    estimates: NSAttributedString(),
    dashboardState: .hidden,
    presentationStep: .hidden,
    progress: 0,
    navigationSearchState: nil,
    errorMessage: nil
  )

  func copyWith(
    transportOptions: [MWMRouterType]? = nil,
    routePoints: NavigationDashboard.RoutePoints? = nil,
    routerType: MWMRouterType? = nil,
    entity: MWMNavigationDashboardEntity? = nil,
    elevationInfo: NavigationDashboard.ElevationInfo?? = nil,
    navigationInfo: NavigationDashboard.NavigationInfo? = nil,
    estimates: NSAttributedString? = nil,
    dashboardState: MWMNavigationDashboardState? = nil,
    presentationStep: NavigationDashboardModalPresentationStep? = nil,
    progress: CGFloat? = nil,
    navigationSearchState: NavigationSearchState? = nil,
    errorMessage: String? = nil
  ) -> NavigationDashboard.ViewModel {
    return NavigationDashboard.ViewModel(
      transportOptions: transportOptions ?? self.transportOptions,
      routePoints: routePoints ?? self.routePoints,
      routerType: routerType ?? self.routerType,
      entity: entity ?? self.entity,
      elevationInfo: elevationInfo ?? self.elevationInfo,
      navigationInfo: navigationInfo ?? self.navigationInfo.copyWith(dashboardState: self.dashboardState),
      estimates: estimates ?? self.estimates,
      dashboardState: dashboardState ?? self.dashboardState,
      presentationStep: presentationStep ?? self.presentationStep,
      progress: progress ?? self.progress,
      navigationSearchState: navigationSearchState,
      errorMessage: errorMessage ?? self.errorMessage
    )
  }

  var startButtonState: StartRouteButton.State {
    if routerType == .ruler || presentationStep == .hidden {
      return .hidden
    }
    if routePoints.count < 2 || routePoints.start == nil || routePoints.finish == nil || dashboardState == .error {
      return .disabled
    }
    if progress < 1 {
      return .loading
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
    case .prepare, .planning, .ready: return .prepare
    default: return .hidden
    }
  }
}

extension CGRect {
  static var screenBounds: CGRect {
    return UIScreen.main.bounds
  }
}
