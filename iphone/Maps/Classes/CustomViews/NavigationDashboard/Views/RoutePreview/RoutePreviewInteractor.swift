extension RoutePreview {
  final class Interactor: NSObject {

    private let presenter: Presenter
    private let router: MWMRouter.Type
    private let searchManager: SearchOnMapManager
    weak var delegate: MWMRoutePreviewDelegate?

    init(presenter: Presenter,
         router: MWMRouter.Type = MWMRouter.self,
         searchManager: SearchOnMapManager = MapViewController.shared()!.searchManager) {
      self.presenter = presenter
      self.router = router
      self.searchManager = searchManager
      super.init()
    }

    func process(_ request: Request) {
      let response = resolve(request)
      presenter.process(response)
    }

    private func resolve(_ event: Request) -> Response {
      print(String(describing: event))
      switch event {
      case .prepareRoute:
        return .none

      case .startRoutePlanning:
        return .show(points: router.points(), routerType: router.type())

      case .routeIsReady:
        return .show(points: router.points(), routerType: router.type())

      case .selectRouterType(let routerType):
        router.setType(routerType)
        router.rebuild(withBestRouter: false)
        return .none

      case let .selectRoutePoint(point, index):
        searchManager.startSearching(isRouting: false)
        if let textToSearch = point?.title {
          searchManager.searchText(textToSearch, isCategory: false)
        }
        return .setHidden(true)

      case .addRoutePoint:
        searchManager.startSearching(isRouting: false)
        return .setHidden(true)

      case .deleteRoutePoint(let point):
        router.removePoint(point)
        let points = router.points()!
        if points.count < 2 {
          router.stopRouting()
        } else {
          router.rebuild(withBestRouter: false)
        }
        return .show(points: points, routerType: router.type())

      case .startNavigation:
        return .showNavigationDashboard

      case let .updateRouteBuildingProgress(progress, routerType):
        return .updateRouteBuildingProgress(progress, routerType: routerType)

      case .updateDrivingOptionState(let state):
        // TODO: implement
        return .none

      case .updateNavigationInfo(let entity):
        return .updateNavigationInfo(entity)

      case let .moveRoutePoint(from, to):
        router.movePoint(at: from, to: to)
        router.rebuild(withBestRouter: false)
        return .show(points: router.points(), routerType: router.type())

      case .updatePresentationFrame(let frame):
        let bottomBound = frame.height - frame.origin.y
        MapViewController.shared()?.setRoutePreviewTopBound(bottomBound, duration: kDefaultAnimationDuration)
        return .none

      case .setHidden(let hidden):
        return .setHidden(hidden)

      case .close:
        router.stopRouting()
        return .close
      }
    }
  }
}

// MARK: - NavigationDashboardView
extension RoutePreview.Interactor: NavigationDashboardView {
  func setHidden(_ hidden: Bool) {
    process(.setHidden(true))
  }
  
  func stateClosed() {
    process(.close)
  }
  
  func onNavigationInfoUpdated(_ entity: MWMNavigationDashboardEntity) {
    process(.updateNavigationInfo(entity))
  }

  func setDrivingOptionState(_ state: MWMDrivingOptionsState) {
    process(.updateDrivingOptionState(state))
  }

  func searchManager(withDidChange state: SearchOnMapState) {
//    switch (state) {
//      case SearchOnMapStateClosed:
//        [self.navigationInfoView setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
//        break;
//      case SearchOnMapStateHidden:
//      case SearchOnMapStateSearching:
//        [self.navigationInfoView setMapSearch];
//    }
    switch state {
    case .closed:
      process(.setHidden(false))
    case .hidden:
      break
    case .searching:
      process(.setHidden(true))
    @unknown default:
      fatalError("Unknown search state: \(state)")
    }
  }

  func updateNavigationInfoAvailableArea(_ frame: CGRect) {
//    print(#function)
    // TODO: navigation
  }

  func setRouteBuilderProgress(_ router: MWMRouterType, progress: CGFloat) {
    process(.updateRouteBuildingProgress(progress, routerType: router))
  }

  func statePrepare() {
    process(.prepareRoute)
  }

  func statePlanning() {
    process(.startRoutePlanning)
  }

  func stateReady() {
    process(.routeIsReady)
  }

  func onRouteStart() {
    process(.startNavigation)
  }

  func onRouteStop() {
    process(.close)
  }

  func onRoutePointsUpdated() {
    process(.startRoutePlanning)
  }

  func stateNavigation() {
    process(.startNavigation) // ?? or onRouteStop
  }

  func stateError(_ errorMessage: String) {
    print(#function)
  }
}
