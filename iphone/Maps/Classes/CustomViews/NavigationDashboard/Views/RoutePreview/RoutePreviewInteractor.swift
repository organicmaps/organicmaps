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
      case .startRouteBuilding:
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
      case .close:
        router.stopRouting()
        return .close
      case .updateRouteBuildingProgress(let progress, routerType: let routerType):
        return .updateRouteBuildingProgress(progress, routerType: routerType)
      case .updateDrivingOptionState(let state):
        // TODO: implement
        return .none
      case .moveRoutePoint(from: let from, to: let to):
        router.movePoint(at: from, to: to)
        router.rebuild(withBestRouter: false)
        return .show(points: router.points(), routerType: router.type())
      case .setHidden(let hidden):
        return .setHidden(hidden)
      }
    }
  }
}

// MARK: - NavigationDashboardView
extension RoutePreview.Interactor: NavigationDashboardView {
  func setHidden(_ hidden: Bool) {
    print(#function)
    process(.setHidden(true))
  }
  
  func stateClosed() {
    print(#function)
    process(.close)
  }
  
  func onNavigationInfoUpdated(_ entity: MWMNavigationDashboardEntity) {
    print(#function)
    // TODO: navigation or ruler

  }

  func setDrivingOptionState(_ state: MWMDrivingOptionsState) {
    print(#function)
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
    print(#function)
    process(.updateRouteBuildingProgress(progress, routerType: router))
  }

  func statePrepare() {
    print(#function)
  }

  func statePlanning() {
    print(#function)
    process(.startRouteBuilding)
  }

  func stateReady() {
    print(#function)
  }

  func onRouteStart() {
    print(#function)
  }

  func onRouteStop() {
    print(#function)
  }

  func onRoutePointsUpdated() {
    print(#function)
    process(.startRouteBuilding)
  }

  func stateNavigation() {
    print(#function)
  }

  func stateError(_ errorMessage: String) {
    print(#function)
  }
}
