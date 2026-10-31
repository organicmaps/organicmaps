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
    .ruler,
  ]

  /// The profiles a track can be followed with, in picker order. The set mirrors the core
  /// GetFollowTrackRouterType() mapping, which follows transit and ruler as pedestrian.
  static let followTrackCases: [MWMRouterType] = [
    .vehicle,
    .pedestrian,
    .bicycle,
  ]

  func image(for isSelected: Bool) -> UIImage {
    let image: UIImage
    let name: String
    switch self {
    case .vehicle:
      image = UIImage(resource: .navigationDashboardCar)
      name = L("routing_type_vehicle")
    case .pedestrian:
      image = UIImage(resource: .navigationDashboardWalk)
      name = L("routing_type_pedestrian")
    case .publicTransport:
      image = UIImage(resource: .navigationDashboardTrain)
      name = L("routing_type_transit")
    case .bicycle:
      image = UIImage(resource: .navigationDashboardBike)
      name = L("routing_type_bicycle")
    case .ruler:
      image = UIImage(resource: .navigationDashboardRuler)
      name = L("routing_type_ruler")
    @unknown default:
      fatalError("Unknown router type")
    }
    let tinted = image.withTintColor(isSelected ? .linkBlue : .blackSecondaryText, renderingMode: .alwaysOriginal)
    tinted.accessibilityLabel = name
    return tinted
  }
}
