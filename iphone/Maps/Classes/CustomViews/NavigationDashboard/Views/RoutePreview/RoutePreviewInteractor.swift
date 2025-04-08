extension RoutePreview {
  final class Interactor: NSObject {

    private let presenter: Presenter
    private let router: MWMRouter.Type
    weak var delegate: MWMRoutePreviewDelegate?

    init(presenter: Presenter, router: MWMRouter.Type = MWMRouter.self) {
      self.presenter = presenter
      self.router = router
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
      case .addRoutePoint:
        // TODO: show search or add point
        return .none
      case .deleteRoutePoint(let point):
        router.removePoint(point)
        return .show(points: router.points(), routerType: router.type())
      case .startNavigation:
        return .showNavigationDashboard
      case .updateRouteBuildingProgress(let progress, routerType: let routerType):
        return .updateRouteBuildingProgress(progress, routerType: routerType)
      case .updateDrivingOptionState(let state):
        // TODO: implement
        return .none
      case .updateNavigationInfo(let entity):
        return .updateNavigationInfo(entity)
      case .moveRoutePoint(from: let from, to: let to):
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
    print(#function)
  }

  func updateNavigationInfoAvailableArea(_ frame: CGRect) {
    print(#function)
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
