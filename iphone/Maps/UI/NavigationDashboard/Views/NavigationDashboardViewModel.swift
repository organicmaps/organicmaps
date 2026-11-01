enum NavigationDashboard {
  /// The presentable state of the followed track. The track may already be deleted while the
  /// follow session keeps navigating a frozen copy of its geometry: the name is nil then.
  struct FollowedTrack: Equatable {
    var isReversed: Bool
    var name: String?
    var lengthMeters: Double

    /// nil when no track is being followed.
    static func current(_ router: MWMRouter.Type) -> Self? {
      guard let info = router.followedTrackInfo() else { return nil }
      // The snapshot outlives its track, so naming it takes an existence check of its own.
      let isTrackAlive = BookmarksManager.shared().hasTrack(info.trackId)
      return FollowedTrack(isReversed: info.isReversed,
                           name: isTrackAlive ? info.name : nil,
                           lengthMeters: info.lengthMeters)
    }
  }

  struct ViewModel: Equatable {
    var transportOptions: [MWMRouterType]
    var routePoints: RoutePoints
    var routerType: MWMRouterType
    var followedTrack: FollowedTrack?
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
    let followedTrack = NavigationDashboard.FollowedTrack.current(MWMRouter.self)
    return NavigationDashboard.ViewModel(
      transportOptions: followedTrack != nil ? MWMRouterType.followTrackCases : MWMRouterType.allCases,
      routePoints: .empty,
      routerType: MWMRouter.renderType(),
      followedTrack: followedTrack,
      entity: MWMNavigationDashboardEntity(),
      trackRecordingState: TrackRecordingManager.shared.recordingState,
      routingOptions: RoutingOptions(routerType: MWMRouter.type()),
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

  var canReverseRoutePoints: Bool {
    // A fixed start leaves a valid preview in RouteNoFollowing, where isRouteBuilt() is false.
    routePoints.hasStartAndFinish &&
      (dashboardState == .ready || dashboardState == .prepare) &&
      progress == 1 &&
      MWMRouter.isRouteValid()
  }

  var isFollowingTrack: Bool { followedTrack != nil }

  /// The single identity line of the followed track ("Name · 12 km"): nil hides it. A deleted
  /// track has no name anymore and shows nothing.
  var followedTrackInfo: String? {
    guard let followedTrack, let name = followedTrack.name, !name.isEmpty else { return nil }
    guard followedTrack.lengthMeters > 0 else { return name }
    return name + " · " + DistanceFormatter.distanceString(fromMeters: followedTrack.lengthMeters)
  }

  var transportOptionsMode: TransportOptionsView.Mode {
    guard followedTrack != nil else { return .standard }
    return .followTrack
  }

  /// Changing a preview profile rebuilds its route: further taps are ignored until the pending rebuild lands (ready, error or cancelled all
  /// leave the planning state or complete the progress).
  var isTransportOptionsSelectionEnabled: Bool {
    !(isFollowingTrack && dashboardState == .planning && progress < 1)
  }

  var startButtonState: StartRouteButton.State {
    if dashboardState == .error {
      return .disabled
    }
    // A followed track defines the route by itself and has no route points.
    if !isFollowingTrack,
       routePoints.count < 2 ||
       routePoints.start == nil ||
       routePoints.finish == nil {
      return .disabled
    }
    if progress < 1 {
      return .loading(progress: progress)
    }
    // The routerType is the render type, which is never ruler/publicTransport while following a track.
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
