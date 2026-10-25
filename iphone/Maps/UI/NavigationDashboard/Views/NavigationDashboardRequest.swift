extension NavigationDashboard {
  enum Request {
    case updateState(MWMNavigationDashboardState)
    case updateRoutePoints
    case startNavigation
    case stopNavigation
    case showError(String)

    case selectRouterType(MWMRouterType)
    case selectRoutePoint(MWMRoutePointSelection)
    case deleteRoutePoint(MWMRoutePoint)
    case moveRoutePoint(from: Int, to: Int)
    case reverseRoutePoints

    case addRoutePointButtonDidTap
    case startButtonDidTap
    case settingsButtonDidTap
    case searchButtonDidTap
    case bookmarksButtonDidTap
    case saveRouteAsTrackButtonDidTap

    case updateRouteBuildingProgress(CGFloat, routerType: MWMRouterType)
    case updateNavigationInfo(MWMNavigationDashboardEntity)
    case updateElevationInfo(RouteElevationPreviewData?, activePointDistance: Double?)
    case updateTrackRecordingState(TrackRecordingState)
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
    .water,
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
    case .water:
      image = UIImage(systemName: "ferry.fill")!
    @unknown default:
      fatalError("Unknown router type")
    }
    image = image.withTintColor(isSelected ? .linkBlue : .blackSecondaryText, renderingMode: .alwaysOriginal)
    if self == .water {
      image.accessibilityLabel = L("water_routing")
    }
    return image
  }
}
