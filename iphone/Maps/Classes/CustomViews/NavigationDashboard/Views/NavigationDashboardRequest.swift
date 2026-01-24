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
    case swapStartAndFinishPoints

    case addRoutePointButtonDidTap
    case startButtonDidTap
    case settingsButtonDidTap
    case searchButtonDidTap
    case bookmarksButtonDidTap
    case saveRouteAsTrackButtonDidTap

    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateNavigationInfo(MWMNavigationDashboardEntity)
    case updateElevationInfo(ElevationInfo?)
    case updateVisibleAreaInsets(UIEdgeInsets)
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
    .ruler,
  ]

  func image(for isSelected: Bool) -> UIImage {
    var image: UIImage
    switch self {
    case .vehicle:
      image = UIImage(resource: .navigationDashboardCar)
    case .pedestrian:
      image = UIImage(resource: .navigationDashboardWalk)
    case .publicTransport:
      image = UIImage(resource: .navigationDashboardTrain)
    case .bicycle:
      image = UIImage(resource: .navigationDashboardBike)
    case .ruler:
      image = UIImage(resource: .navigationDashboardRuler)
    @unknown default:
      fatalError("Unknown router type")
    }
    if #available(iOS 13, *) {
      return image.withTintColor(isSelected ? .linkBlue() : .blackSecondaryText(), renderingMode: .alwaysOriginal)
    }
    return image
  }
}
