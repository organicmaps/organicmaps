extension NavigationDashboard {
  final class Interactor: NSObject {
    private let presenter: Presenter
    private let router: MWMRouter.Type
    private let mapViewController: MapViewController
    private let searchManager: SearchOnMapManager

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

    func process(_ request: Request) {
      let response = resolve(request)
      presenter.process(response)
    }

    private func resolve(_ event: Request) -> Response {
      switch event {
      case .updateState(let state):
        return .updateState(state)

      case .updateRoutePoints:
        return .show(points: router.points(), routerType: router.type())

      case .selectRouterType(let routerType):
        router.setType(routerType)
        router.rebuild(withBestRouter: false)
        return .none

      case .selectRoutePoint(let point):
        delegate?.routePreviewDidSelect(point, shouldAppend: false)
        searchManager.startSearching(isRouting: false)
        if let point, !point.isMyPosition, let textToSearch = point.title {
          searchManager.searchText(SearchQuery(textToSearch, source: .typedText))
        }
        return .setHidden(true)

      case .addRoutePointButtonDidTap:
        delegate?.routePreviewDidSelect(nil, shouldAppend: true)
        searchManager.startSearching(isRouting: false)
        return .setHidden(true)

      case .deleteRoutePoint(let point):
        router.removePoint(point)
        router.rebuild(withBestRouter: false)
        return .show(points: router.points()!, routerType: router.type())

      case .startNavigation:
        return .showNavigationDashboard

      case .stopNavigation:
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

      case .updateElevationInfo(let elevationInfo):
        return .updateElevationInfo(elevationInfo)

      case .updateNavigationInfoAvailableArea(let frame):
        return .updateNavigationInfoAvailableArea(frame)

      case .updateSearchState(let state):
        if state == .closed {
          delegate?.routePreviewDidSelect(nil, shouldAppend: false)
        }
        return .updateSearchState(state)

      case .updateDrivingOptionsState:
        let routingOptions = RoutingOptions()
        return .updateDrivingOptionsState(routingOptions)

      case .moveRoutePoint(let from, let to):
        router.movePoint(at: from, to: to)
        router.rebuild(withBestRouter: false)
        return .show(points: router.points(), routerType: router.type())

      case .swapStartAndFinishPoints:
        router.swapStartAndFinish()
        return .show(points: router.points(), routerType: router.type())

      case .updateVisibleAreaInsets(let insets):
        mapViewController.updateVisibleAreaInsets(for: self, insets: insets, updatingViewport: true)
        return .none

      case .setHidden(let hidden):
        return .setHidden(hidden)

      case .didUpdatePresentationStep(let step):
        return .updatePresentationStep(step)

      case .close:
        router.stopRouting()
        return .close
      }
    }
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
    let elevationInfo = router.routeElevationProfileData()
    process(.updateElevationInfo(elevationInfo))
  }
}
