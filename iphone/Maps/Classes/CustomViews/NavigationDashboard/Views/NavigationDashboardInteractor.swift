extension NavigationDashboard {
  final class Interactor: NSObject {
    private let presenter: Presenter
    private let router: MWMRouter.Type
    private let mapViewController: MapViewController
    private let searchManager: SearchOnMapManager
    private let frameworkHelper: FrameworkHelper.Type

    weak var delegate: MWMRoutePreviewDelegate?

    init(presenter: Presenter,
         router: MWMRouter.Type = MWMRouter.self,
         mapViewController: MapViewController = MapViewController.shared()!,
         searchManager: SearchOnMapManager = MapViewController.shared()!.searchManager,
         frameworkHelper: FrameworkHelper.Type = FrameworkHelper.self) {
      self.presenter = presenter
      self.router = router
      self.mapViewController = mapViewController
      self.searchManager = searchManager
      self.frameworkHelper = frameworkHelper
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

      case let .selectRoutePoint(point):
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
        frameworkHelper.saveRouteAsTrack()
        return .setRouteAsTrackSaved

      case let .updateRouteBuildingProgress(progress, routerType):
        return .updateRouteBuildingProgress(progress, routerType: routerType)

      case .updateNavigationInfo(let entity):
        return .updateNavigationInfo(entity)

      case .updateElevationInfo(let elevationInfo):
        return .updateElevationInfo(elevationInfo)

      case .updateNavigationInfoAvailableArea(let frame):
        return .updateNavigationInfoAvailableArea(frame)

      case .updateSearchState(let state):
        if state == .closed {
          delegate?.routePreviewDidSelect(nil, shouldAppend: false)
        }
        return .updateSearchState(state)

      case .updateDrivingOptionsState(_):
        let routingOptions = RoutingOptions()
        return .updateDrivingOptionsState(routingOptions)

      case let .moveRoutePoint(from, to):
        router.movePoint(at: from, to: to)
        router.rebuild(withBestRouter: false)
        return .show(points: router.points(), routerType: router.type())

      case .updatePresentationFrame(let frame):
        let bottomBound = frame.height - frame.origin.y
        mapViewController.setRoutePreviewTopBound(bottomBound)
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

  // TODO: (KK) elevation info should be removed when the new elevation chart with all statistics will be implemented
  static var elevationAttributes: [NSAttributedString.Key: Any] {
    [.foregroundColor: UIColor.blackSecondaryText(), .font: UIFont.medium16()]
  }

  private func buildElevationInfoIfNeeded() {
    guard router.hasRouteAltitude() else {
      presenter.process(.updateElevationInfo(nil))
      return
    }
    router.routeAltitudeImage(
      for: CGSize(width: 350, height: 50)) { [weak self] image, totalAscent, totalDescent in
        guard let self else { return }
        guard let totalAscent, let totalDescent else {
          self.process(.updateElevationInfo(nil))
          return
        }
        let attributes = Self.elevationAttributes
        let elevation = NSMutableAttributedString(string: "")
        elevation.append(MWMNavigationDashboardEntity.estimateDot())
        elevation.append(NSAttributedString(string: "▲ \(totalAscent)  ", attributes: attributes))
        elevation.append(NSAttributedString(string: "▼ \(totalDescent)", attributes: attributes))
        let elevationInfo = NavigationDashboard.ElevationInfo(estimates: elevation, image: image)
        self.process(.updateElevationInfo(elevationInfo))
      }
  }
}
