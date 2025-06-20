extension NavigationDashboard {
  enum Request {
    case updateState(MWMNavigationDashboardState)
    case updateRoutePoints
    case startNavigation
    case stopNavigation
    case showError(String)

    case selectRouterType(MWMRouterType)
    case selectRoutePoint(MWMRoutePoint?)
    case deleteRoutePoint(MWMRoutePoint)
    case moveRoutePoint(from: Int, to: Int)
    case addRoutePointButtonDidTap
    case startButtonDidTap
    case settingsButtonDidTap

    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateNavigationInfo(MWMNavigationDashboardEntity)
    case updateElevationInfo(ElevationInfo?)
    case updatePresentationFrame(CGRect)
    case updateNavigationInfoAvailableArea(CGRect)
    case updateSearchState(SearchOnMapState)
    case updateDrivingOptionsState(MWMDrivingOptionsState)

    case didUpdatePresentationStep(NavigationDashboardModalPresentationStep)
    case setHidden(Bool)
    case goBack
    case close
  }
}

extension MWMRouterType: CaseIterable {
  public static var allCases: [MWMRouterType] = [
    .vehicle,
    .pedestrian,
    .publicTransport,
    .bicycle,
    .ruler
  ]

  func image(for isSelected: Bool) -> UIImage {
    switch self {
    case .vehicle:
      UIImage(resource: isSelected ? .icCarSelected : .icCar)
    case .pedestrian:
      UIImage(resource: isSelected ? .icPedestrianSelected : .icPedestrian)
    case .publicTransport:
      UIImage(resource: isSelected ? .icTrainSelected : .icTrain)
    case .bicycle:
      UIImage(resource: isSelected ? .icBikeSelected : .icBike)
    case .ruler:
      UIImage(resource: isSelected ? .icRulerRouteSelected : .icRulerRoute)
    @unknown default:
      fatalError("Unknown router type")
    }
  }
}
