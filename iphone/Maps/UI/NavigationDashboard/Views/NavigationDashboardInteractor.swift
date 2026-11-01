extension NavigationDashboard {
  final class Interactor: NSObject {
    private let presenter: Presenter
    private let router: MWMRouter.Type
    private let mapViewController: MapViewController
    private let searchManager: SearchOnMapManager
    private var routeElevationActivePointDistance: Double?
    private var presentationStep: NavigationDashboardModalPresentationStep = .hidden

    weak var delegate: MWMRoutePreviewDelegate?

    init(presenter: Presenter,
         router: MWMRouter.Type = MWMRouter.self,
         mapViewController: MapViewController = MapViewController.shared()!,
         searchManager: SearchOnMapManager = MapViewController.shared()!.searchManager) {
      self.presenter = presenter
      self.router = router
      self.mapViewController = mapViewController
      self.searchManager = searchManager
      super.init()
    }

    private var isValid = true

    func invalidate() {
      isValid = false
    }

    func process(_ request: Request) {
      guard isValid else { return }
      let response = resolve(request)
      presenter.process(response)
    }

    /// The render router type resolves a followed track to its follow profile, so the dashboard
    /// never presents or gates on ruler/publicTransport while a track is being followed.
    private var showRoutePoints: Response {
      .show(points: router.points(), routerType: router.renderType(), followedTrack: .current(router))
    }

    private func resolve(_ event: Request) -> Response {
      switch event {
      case .updateState(let state):
        return .updateState(state)

      case .updateRoutePoints:
        return showRoutePoints

      case .selectRouterType(let routerType):
        guard router.isFollowingTrack() else {
          router.setType(routerType)
          router.rebuild(withBestRouter: false)
          return .none
        }
        return switchFollowedTrackProfile(to: routerType)

      case .selectRoutePoint(let selection):
        delegate?.routePreviewDidSelect(selection)
        searchManager.startSearching(isRouting: true)
        return .setHidden(true)

      case .addRoutePointButtonDidTap:
        guard !router.isRoutePointsLimitReached() else {
          Toast.show(withText: L("routing_max_stops_reached"))
          return .none
        }
        let selection = MWMRoutePointSelection(point: nil,
                                               type: .intermediate,
                                               shouldAppend: true)
        delegate?.routePreviewDidSelect(selection)
        searchManager.startSearching(isRouting: true)
        return .setHidden(true)

      case .deleteRoutePoint(let point):
        router.removePointAndRebuild(point)
        return showRoutePoints

      case .startNavigation:
        clearRouteElevationActivePoint()
        return .showNavigationDashboard

      case .stopNavigation:
        clearRouteElevationActivePoint()
        return .close

      case .showError(let errorMessage):
        return .showError(errorMessage)

      case .startButtonDidTap:
        delegate?.routingStartButtonDidTap()
        return .none

      case .settingsButtonDidTap:
        delegate?.routePreviewDidPressDrivingOptions()
        return .none

      case .searchButtonDidTap:
        searchManager.startSearching(isRouting: false)
        return .setHidden(true)

      case .bookmarksButtonDidTap:
        mapViewController.bookmarksCoordinator.open()
        return .none

      case .saveRouteAsTrackButtonDidTap:
        router.saveRouteAsTrack()
        return .setRouteAsTrackSaved

      case .reverseTrackDirectionButtonDidTap:
        // The rebuild needs the track itself, so it fails once the followed track has been
        // deleted; the current follow session stays untouched then — but the header must stop
        // advertising the stale identity, so refresh instead of returning .none.
        let result = router.reverseFollowedTrack()
        if result == .busy {
          return .none
        }
        if result != .ok {
          Toast.show(withText: followTrackErrorMessage(result))
          return showRoutePoints
        }
        return .none

      case .updateRouteBuildingProgress(let progress, let routerType):
        return .updateRouteBuildingProgress(progress, routerType: routerType)

      case .updateNavigationInfo(let entity):
        return .updateNavigationInfo(entity)

      case .updateTrackRecordingState(let state):
        return .updateTrackRecordingState(state)

      case .updateElevationInfo(let elevationInfo, let activePointDistance):
        return .updateElevationInfo(elevationInfo, activePointDistance: activePointDistance)

      case .updateNavigationInfoAvailableArea(let frame):
        return .updateNavigationInfoAvailableArea(frame)

      case .updateSearchState(let state):
        if state == .closed {
          delegate?.routePreviewDidCancelPointSelection()
        }
        return .updateSearchState(state)

      case .updateDrivingOptionsState:
        let routingOptions = RoutingOptions(routerType: router.type())
        return .updateDrivingOptionsState(routingOptions)

      case .moveRoutePoint(let from, let to):
        router.movePoint(at: from, to: to)
        router.rebuild(withBestRouter: false)
        return showRoutePoints

      case .reverseRoutePoints:
        router.reverseRoute()
        return showRoutePoints

      case .updateVisibleAreaInsets(let insets):
        mapViewController.updateVisibleAreaInsets(for: self, insets: insets, updatingViewport: true)
        return .none

      case .setHidden(let hidden):
        return .setHidden(hidden)

      case .didUpdatePresentationStep(let step):
        let shouldRestoreActivePoint = presentationStep == .hidden && step != .hidden
        presentationStep = step
        if shouldRestoreActivePoint {
          restoreRouteElevationActivePoint()
        }
        return .updatePresentationStep(step)

      case .close:
        router.stopRouting()
        return .close
      }
    }

    /// Switching the profile of a follow session. The core validates the re-follow before it
    /// touches the running session and keeps the picked profile in this preview, so a refusal only
    /// leaves the current (frozen) session in place.
    private func switchFollowedTrackProfile(to routerType: MWMRouterType) -> Response {
      guard routerType != router.renderType() else { return .none }
      let result = router.setFollowTrackProfile(routerType)
      if result != .ok, result != .busy {
        Toast.show(withText: followTrackErrorMessage(result))
      }
      // Refresh either way: after a success to restore the reduced picker selection through the
      // nested rebuild events, after a refusal to snap the picker selection back and to drop the
      // identity line of a track that is gone.
      return showRoutePoints
    }

    /// A track that has been deleted mid-session needs its own message: the generic planning
    /// error would blame the routing engine for a track that is simply gone.
    private func followTrackErrorMessage(_ result: MWMFollowTrackResult) -> String {
      if result == .noRemainingGeometry {
        return L("track_no_remaining_geometry")
      }
      return L(result == .noTrack ? "track_is_not_available" : "routing_planning_error")
    }

    fileprivate func clearRouteElevationActivePoint() {
      routeElevationActivePointDistance = nil
      router.resetRouteElevationActivePoint()
    }

    fileprivate func restoreRouteElevationActivePoint() {
      guard let routeElevationActivePointDistance else { return }
      router.setRouteElevationActivePointDistance(routeElevationActivePointDistance)
    }
  }
}

// MARK: - ElevationProfileViewControllerDelegate

extension NavigationDashboard.Interactor: ElevationProfileViewControllerDelegate {
  func updateMapPoint(distance: Double) {
    routeElevationActivePointDistance = distance
    router.setRouteElevationActivePointDistance(distance)
  }
}

// MARK: - NavigationDashboardView

extension NavigationDashboard.Interactor: NavigationDashboardView {
  func setHidden(_ hidden: Bool) {
    process(.setHidden(hidden))
  }

  func onNavigationInfoUpdated(_ entity: MWMNavigationDashboardEntity) {
    process(.updateNavigationInfo(entity))
  }

  func setTrackRecordingState(_ state: TrackRecordingState) {
    process(.updateTrackRecordingState(state))
  }

  func setDrivingOptionState(_ state: MWMDrivingOptionsState) {
    process(.updateDrivingOptionsState(state))
  }

  func searchManager(withDidChange state: SearchOnMapState) {
    process(.updateSearchState(state))
  }

  func updateNavigationInfoAvailableArea(_ frame: CGRect) {
    process(.updateNavigationInfoAvailableArea(frame))
  }

  func setRouteBuilderProgress(_ router: MWMRouterType, progress: CGFloat) {
    process(.updateRouteBuildingProgress(progress, routerType: router))
  }

  func statePrepare() {
    clearRouteElevationActivePoint()
    process(.updateState(.prepare))
  }

  func statePlanning() {
    process(.updateState(.planning))
    process(.updateRoutePoints)
  }

  func stateReady() {
    process(.updateState(.ready))
    buildElevationInfoIfNeeded()
    process(.updateRoutePoints)
  }

  func stateClosed() {
    process(.updateState(.closed))
    process(.stopNavigation)
  }

  func onRouteStart() {
    process(.startNavigation)
  }

  func onRouteStop() {
    process(.stopNavigation)
  }

  func onRoutePointsUpdated() {
    process(.updateRoutePoints)
  }

  func stateNavigation() {
    process(.updateState(.navigation))
    process(.startNavigation)
  }

  func stateError(_ errorMessage: String) {
    process(.updateState(.error))
    process(.showError(errorMessage))
  }

  private func buildElevationInfoIfNeeded() {
    guard let elevationInfo = router.routeElevationProfileData() else {
      clearRouteElevationActivePoint()
      process(.updateElevationInfo(nil, activePointDistance: nil))
      return
    }

    let maxDistance = elevationInfo.elevationProfileData?.points.last?.distance ?? 0
    let activePointDistance = routeElevationActivePointDistance.map { min(max(0, $0), maxDistance) }
    routeElevationActivePointDistance = activePointDistance
    process(.updateElevationInfo(elevationInfo, activePointDistance: activePointDistance))
    // Restore on the next run loop because Place Page finishes deselecting its map marker after the route-ready callback.
    DispatchQueue.main.async { [weak self] in
      self?.restoreRouteElevationActivePoint()
    }
  }
}
