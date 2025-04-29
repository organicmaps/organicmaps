extension RoutePreview {
  
  struct ViewModel: Equatable {
    let transportOptions: [MWMRouterType]
    let routePoints: RoutePoints
    let routerType: MWMRouterType
    let entity: MWMNavigationDashboardEntity
    let elevationInfo: ElevationInfo?
    let navigationInfo: NavigationInfo
    let estimates: NSAttributedString
    let dashboardState: MWMNavigationDashboardState
    let presentationStep: ModalPresentationStep
    let shouldClose: Bool
    let progress: CGFloat
    let navigationSearchState: NavigationSearchState?
  }
  
  struct ElevationInfo: Equatable {
    let estimates: NSAttributedString
    let image: UIImage?
  }

  struct NavigationInfo: Equatable {
    let state: MWMNavigationInfoViewState
    let availableArea: CGRect
    let shouldUpdateToastView: Bool

    var isControlsVisible: Bool { state == .navigation }
  }
}

extension RoutePreview.ViewModel {
  static let initial = RoutePreview.ViewModel(
    transportOptions: MWMRouterType.allCases,
    routePoints: .empty,
    routerType: .vehicle,
    entity: MWMNavigationDashboardEntity(),
    elevationInfo: nil,
    navigationInfo: .hidden,
    estimates: NSAttributedString(),
    dashboardState: .hidden,
    presentationStep: .hidden,
    shouldClose: false,
    progress: 0,
    navigationSearchState: nil
  )

  func copyWith(
    transportOptions: [MWMRouterType]? = nil,
    routePoints: RoutePreview.RoutePoints? = nil,
    routerType: MWMRouterType? = nil,
    entity: MWMNavigationDashboardEntity? = nil,
    elevationInfo: RoutePreview.ElevationInfo?? = nil,
    navigationInfo: RoutePreview.NavigationInfo? = nil,
    estimates: NSAttributedString? = nil,
    dashboardState: MWMNavigationDashboardState? = nil,
    presentationStep: ModalPresentationStep? = nil,
    shouldClose: Bool? = nil,
    progress: CGFloat? = nil,
    navigationSearchState: NavigationSearchState? = nil
  ) -> RoutePreview.ViewModel {
    return RoutePreview.ViewModel(
      transportOptions: transportOptions ?? self.transportOptions,
      routePoints: routePoints ?? self.routePoints,
      routerType: routerType ?? self.routerType,
      entity: entity ?? self.entity,
      elevationInfo: elevationInfo ?? self.elevationInfo,
      navigationInfo: navigationInfo ?? self.navigationInfo,
      estimates: estimates ?? self.estimates,
      dashboardState: dashboardState ?? self.dashboardState,
      presentationStep: presentationStep ?? self.presentationStep,
      shouldClose: shouldClose ?? self.shouldClose,
      progress: progress ?? self.progress,
      navigationSearchState: navigationSearchState
    )
  }

  var startButtonState: StartRouteButton.State {
    if routerType == .ruler || presentationStep == .hidden {
      return .hidden
    }
    if routePoints.count < 2 || routePoints.start == nil || routePoints.finish == nil {
      return .disabled
    }
    if progress < 1 {
      return .loading
    }
    return .enabled
  }
}

extension RoutePreview.NavigationInfo {
  static let hidden = RoutePreview.NavigationInfo(
    state: .hidden,
    availableArea: .screenBounds,
    shouldUpdateToastView: false
  )

  func copyWith(
    dashboardState: MWMNavigationDashboardState? = nil,
    availableArea: CGRect? = nil,
    shouldUpdateToastView: Bool? = nil
  ) -> RoutePreview.NavigationInfo {
    return RoutePreview.NavigationInfo(
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
