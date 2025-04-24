extension RoutePreview {
  struct ViewModel: Equatable {
    let transportOptions: [MWMRouterType]
    let routePoints: RoutePoints
    let routerType: MWMRouterType
    let entity: MWMNavigationDashboardEntity
    let elevationInfo: ElevationInfo?
    let estimates: NSAttributedString
    let state: MWMNavigationDashboardState
    let presentationStep: ModalPresentationStep
    let shouldClose: Bool
    let progress: CGFloat
  }

  struct ElevationInfo: Equatable {
    let estimates: NSAttributedString
    let image: UIImage?
  }
}

extension RoutePreview.ViewModel {
  static let initial = RoutePreview.ViewModel(
    transportOptions: MWMRouterType.allCases,
    routePoints: .empty,
    routerType: .vehicle,
    entity: MWMNavigationDashboardEntity(),
    elevationInfo: nil,
    estimates: NSAttributedString(),
    state: .hidden,
    presentationStep: .hidden,
    shouldClose: false,
    progress: 0
  )

  func copy(
    transportOptions: [MWMRouterType]? = nil,
    routePoints: RoutePreview.RoutePoints? = nil,
    routerType: MWMRouterType? = nil,
    entity: MWMNavigationDashboardEntity? = nil,
    elevationInfo: RoutePreview.ElevationInfo?? = nil,
    estimates: NSAttributedString? = nil,
    state: MWMNavigationDashboardState? = nil,
    presentationStep: ModalPresentationStep? = nil,
    shouldClose: Bool? = nil,
    progress: CGFloat? = nil
  ) -> RoutePreview.ViewModel {
    return RoutePreview.ViewModel(
      transportOptions: transportOptions ?? self.transportOptions,
      routePoints: routePoints ?? self.routePoints,
      routerType: routerType ?? self.routerType,
      entity: entity ?? self.entity,
      elevationInfo: elevationInfo ?? self.elevationInfo,
      estimates: estimates ?? self.estimates,
      state: state ?? self.state,
      presentationStep: presentationStep ?? self.presentationStep,
      shouldClose: shouldClose ?? self.shouldClose,
      progress: progress ?? self.progress
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
