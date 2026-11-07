enum NavigationDashboard {
  struct ViewModel: Equatable {
    var planningMode: PlanningMode { routePoints.planningMode }
    var transportOptions: [MWMRouterType] { planningMode.transportOptions }
    var routePoints: RoutePoints
    var routerType: MWMRouterType
    var entity: MWMNavigationDashboardEntity
    var trackRecordingState: TrackRecordingState
    var routingOptions: RoutingOptions
    var routeElevationPreviewData: RouteElevationPreviewData?
    var routeElevationActivePointDistance: Double?
    var navigationInfo: NavigationInfo
    var estimates: NSAttributedString
    var dashboardState: MWMNavigationDashboardState
    var presentationStep: NavigationDashboardModalPresentationStep {
      didSet {
        if presentationStep != .hidden {
          latestVisiblePresentationStep = presentationStep
        }
      }
    }

    var latestVisiblePresentationStep: NavigationDashboardModalPresentationStep
    var progress: CGFloat
    var navigationSearchState: NavigationSearchState?
    var canSaveRouteAsTrack: Bool
    var errorMessage: String?
  }

  struct NavigationInfo: Equatable {
    var state: MWMNavigationInfoViewState
    var availableArea: CGRect
  }
}

extension NavigationDashboard.ViewModel {
  static var initial: Self {
    NavigationDashboard.ViewModel(
      routePoints: MWMRouter.isFollowingTrack()
        ? .init(trackName: MWMRouter.followedTrackInfo()?.name ?? "") : .empty,
      routerType: MWMRouter.renderType(),
      entity: MWMNavigationDashboardEntity(),
      trackRecordingState: TrackRecordingManager.shared.recordingState,
      routingOptions: RoutingOptions(routerType: MWMRouter.renderType()),
      routeElevationPreviewData: nil,
      routeElevationActivePointDistance: nil,
      navigationInfo: .hidden,
      estimates: NSAttributedString(),
      dashboardState: .hidden,
      presentationStep: .hidden,
      latestVisiblePresentationStep: isiPad ? .regular : .compact,
      progress: 0,
      navigationSearchState: nil,
      canSaveRouteAsTrack: false,
      errorMessage: nil
    )
  }

  var isBottomActionsMenuHidden: Bool { presentationStep == .hidden }

  var isTransportSelectionEnabled: Bool {
    guard planningMode == .navigateTrack, dashboardState == .planning else { return true }
    return progress >= 1
  }

  var supportsElevationProfile: Bool {
    planningMode == .navigateTrack || routerType == .pedestrian || routerType == .bicycle
  }

  var canReverseRoutePoints: Bool {
    if planningMode == .navigateTrack {
      return routePoints.hasStartAndFinish && dashboardState == .ready && progress == 1
    }
    // A fixed start leaves a valid preview in RouteNoFollowing, where isRouteBuilt() is false.
    return routePoints.hasStartAndFinish &&
      (dashboardState == .ready || dashboardState == .prepare) &&
      progress == 1 &&
      MWMRouter.isRouteValid()
  }

  var startButtonState: StartRouteButton.State {
    if !routePoints.hasStartAndFinish ||
      dashboardState == .error {
      return .disabled
    }
    if progress < 1 {
      return .loading(progress: progress)
    }
    let isStartEnabled = routerType != .ruler && routerType != .publicTransport
    return isStartEnabled ? .enabled : .disabled
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
    availableArea: MapViewController.shared()?.navigationInfoArea.areaFrame ?? .screenBounds
  )
}

extension CGRect {
  static var screenBounds: CGRect {
    UIScreen.main.bounds
  }
}

extension RoutingOptions {
  var enabledOptionsCount: Int {
    [avoidToll, avoidDirty, avoidFerry, avoidMotorway].filter { $0 }.count
  }
}
