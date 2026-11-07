extension NavigationDashboard {
  final class Interactor: NSObject {
    private let presenter: Presenter
    private let router: MWMRouter.Type
    private let mapViewController: MapViewController
    private let searchManager: SearchOnMapManager
    private var routeElevationActivePointDistance: Double?
    private var pendingEntryDistance: Double?
    private var presentationStep: NavigationDashboardModalPresentationStep = .hidden
    private var isValid = true

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

    func invalidate() {
      pendingEntryDistance = nil
      isValid = false
    }

    func process(_ request: Request) {
      guard isValid else { return }
      let response = resolve(request)
      presenter.process(response)
    }

    private var planningMode: PlanningMode { router.isFollowingTrack() ? .navigateTrack : .default }

    private func resolve(_ event: Request) -> Response {
      switch event {
      case .updateState(let state):
        return .updateState(state)

      case .updateRoutePoints:
        return showRoutePoints()

      case .selectRouterType(let routerType):
        guard planningMode.transportOptions.contains(routerType) else { return .none }
        if router.isFollowingTrack() {
          let result = router.setFollowTrackProfile(routerType)
          showTrackError(result)
          return showRoutePoints()
        }
        router.setType(routerType)
        router.rebuild(withBestRouter: false)
        return .none

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
        return showRoutePoints()

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
        let routingOptions = RoutingOptions(routerType: router.renderType())
        return .updateDrivingOptionsState(routingOptions)

      case .moveRoutePoint(let from, let to):
        router.movePoint(at: from, to: to)
        router.rebuild(withBestRouter: false)
        return showRoutePoints()

      case .reverseRoutePoints:
        if router.isFollowingTrack() {
          showTrackError(router.reverseFollowedTrack())
        } else {
          router.reverseRoute()
        }
        return showRoutePoints()

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

    private func showRoutePoints() -> Response {
      let points = router.isFollowingTrack()
        ? RoutePoints(trackName: router.followedTrackInfo()?.name ?? "")
        : RoutePoints(points: router.points())
      return .show(points: points, routerType: router.renderType())
    }

    private func updateTrackEntry(atDistance distance: Double) -> MWMFollowTrackResult {
      let result = router.updateFollowedTrackEntry(atDistance: distance)
      pendingEntryDistance = result == .busy ? distance : nil
      if result != .ok, result != .busy {
        routeElevationActivePointDistance = router.routeElevationActivePointDistance()?.doubleValue
      }
      showTrackError(result)
      return result
    }

    private func showTrackError(_ result: MWMFollowTrackResult) {
      guard result != .ok, result != .busy else { return }
      Toast.show(withText: L("routing_planning_error"))
    }

    fileprivate func clearRouteElevationActivePoint() {
      guard isValid else { return }
      pendingEntryDistance = nil
      routeElevationActivePointDistance = nil
      router.resetRouteElevationActivePoint()
    }

    fileprivate func restoreRouteElevationActivePoint() {
      guard isValid, let routeElevationActivePointDistance else { return }
      router.setRouteElevationActivePointDistance(routeElevationActivePointDistance)
    }
  }
}

// MARK: - ElevationProfileViewControllerDelegate

extension NavigationDashboard.Interactor: ElevationProfileViewControllerDelegate {
  func updateMapPoint(distance: Double) {
    guard isValid else { return }
    routeElevationActivePointDistance = distance
    router.setRouteElevationActivePointDistance(distance)
  }

  func didFinishSelectingPoint(distance: Double) {
    guard isValid, router.isFollowingTrack(), !router.isOnRoute() else { return }
    let result = updateTrackEntry(atDistance: distance)
    if result != .ok, result != .busy {
      buildElevationInfoIfNeeded()
    }
  }

  func didCancelSelectingPoint() {
    guard isValid, router.isFollowingTrack(), !router.isOnRoute() else { return }
    routeElevationActivePointDistance = pendingEntryDistance ?? router.routeElevationActivePointDistance()?.doubleValue
    buildElevationInfoIfNeeded()
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
    if !router.isFollowingTrack() {
      clearRouteElevationActivePoint()
    }
    process(.updateState(.prepare))
  }

  func statePlanning() {
    process(.updateState(.planning))
    process(.updateRoutePoints)
  }

  func stateReady() {
    guard isValid else { return }
    if let distance = pendingEntryDistance {
      pendingEntryDistance = nil
      if router.isFollowingTrack(), !router.isOnRoute() {
        let result = updateTrackEntry(atDistance: distance)
        if result == .ok || result == .busy {
          return
        }
      }
    }
    process(.updateState(.ready))
    buildElevationInfoIfNeeded()
    process(.updateRoutePoints)
  }

  func stateClosed() {
    guard isValid else { return }
    clearRouteElevationActivePoint()
    process(.updateState(.closed))
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
    pendingEntryDistance = nil
    process(.updateState(.error))
    process(.showError(errorMessage))
  }

  private func buildElevationInfoIfNeeded() {
    guard isValid else { return }
    guard let elevationInfo = router.routeElevationProfileData() else {
      clearRouteElevationActivePoint()
      process(.updateElevationInfo(nil, activePointDistance: nil))
      return
    }

    let maxDistance = elevationInfo.elevationProfileData?.points.last?.distance ?? 0
    let selectedDistance = routeElevationActivePointDistance ?? router.routeElevationActivePointDistance()?.doubleValue
    let activePointDistance = selectedDistance.map { min(max(0, $0), maxDistance) }
    routeElevationActivePointDistance = activePointDistance
    process(.updateElevationInfo(elevationInfo, activePointDistance: activePointDistance))
    // Restore on the next run loop because Place Page finishes deselecting its map marker after the route-ready callback.
    DispatchQueue.main.async { [weak self] in
      self?.restoreRouteElevationActivePoint()
    }
  }
}
