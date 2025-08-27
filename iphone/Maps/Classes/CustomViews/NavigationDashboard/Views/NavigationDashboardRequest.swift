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
    case searchButtonDidTap
    case bookmarksButtonDidTap
    case saveRouteAsTrackButtonDidTap

    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateNavigationInfo(MWMNavigationDashboardEntity)
    case updateElevationInfo(ElevationInfo?)
    case updatePresentationFrame(CGRect)
    case updateNavigationInfoAvailableArea(CGRect)
    case updateSearchState(SearchOnMapState)
    case updateDrivingOptionsState(MWMDrivingOptionsState)

    case didUpdatePresentationStep(NavigationDashboardModalPresentationStep)
    case setHidden(Bool)
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
    if #available(iOS 15, *) {
      switch self {
      case .vehicle:
        UIImage(resource: .navigationDashboardCar)
      case .pedestrian:
        UIImage(resource: .navigationDashboardWalk)
      case .publicTransport:
        UIImage(resource: .navigationDashboardTrain)
      case .bicycle:
        UIImage(resource: .navigationDashboardBike)
      case .ruler:
        UIImage(resource: .navigationDashboardRuler)
      @unknown default:
        fatalError("Unknown router type")
      }
    } else {
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
}
