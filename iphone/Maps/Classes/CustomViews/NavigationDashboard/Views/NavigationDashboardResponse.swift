extension NavigationDashboard {
  enum Response: Equatable {
    case none
    case updateState(MWMNavigationDashboardState)
    case show(points: [MWMRoutePoint], routerType: MWMRouterType)

    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateNavigationInfo(MWMNavigationDashboardEntity)
    case updateElevationInfo(ElevationInfo?)
    case updatePresentationStep(NavigationDashboardModalPresentationStep)
    case updateNavigationInfoAvailableArea(CGRect)
    case updateSearchState(SearchOnMapState)
    case updateDrivingOptionsState(MWMDrivingOptionsState)

    case showNavigationDashboard
    case setHidden(Bool)
    case showError(String)
    case close
  }
}
