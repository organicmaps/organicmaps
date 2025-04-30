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
        searchManager.startSearching(isRouting: false)
        if let textToSearch = point?.title {
          searchManager.searchText(textToSearch, isCategory: false)
        }
        return .setHidden(true)

      case .addRoutePointButtonDidTap:
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

      case .startButtonDidTap:
        delegate?.routingStartButtonDidTap()
        return .none

      case .settingsButtonDidTap:
        delegate?.settingsButtonDidTap()
        return .none

      case let .updateRouteBuildingProgress(progress, routerType):
        return .updateRouteBuildingProgress(progress, routerType: routerType)

      case .updateDrivingOptionState(let state):
        // TODO: implement
        return .none

      case .updateNavigationInfo(let entity):
        return .updateNavigationInfo(entity)

      case .updateElevationInfo(let elevationInfo):
        return .updateElevationInfo(elevationInfo)

      case .updateNavigationInfoAvailableArea(let frame):
        return .updateNavigationInfoAvailableArea(frame)

      case .updateSearchState(let state):
        return .updateSearchState(state)
        
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

      case .didUpdatePresentationStep(let step):
        return .updatePresentationStep(step)

      case .goBack:
        router.stopRouting()
        return .goBack

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

  func onNavigationInfoUpdated(_ entity: MWMNavigationDashboardEntity) {
    process(.updateNavigationInfo(entity))
  }

  func setDrivingOptionState(_ state: MWMDrivingOptionsState) {
    process(.updateDrivingOptionState(state))
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
    print(#function, errorMessage)
  }

  // TODO: (KK) elevation info should be removed when the new elevation chart with all statistics will be implemented
  private func buildElevationInfoIfNeeded() {
    guard router.hasRouteAltitude() else {
      presenter.process(.updateElevationInfo(nil))
      return
    }
    router.routeAltitudeImage(
      for: /*heightProfileImage.frame.size*/ CGSize(width: 350, height: 50)) { [weak self] image, totalAscent, totalDescent in
        guard let self else { return }
        guard let totalAscent, let totalDescent else {
          self.process(.updateElevationInfo(nil))
          return
        }
        let attributes = BaseRoutePreviewStatus.elevationAttributes
        let ascentImageString: NSAttributedString
        let descentImageString: NSAttributedString
        if #available(iOS 13.0, *) {
          let imageFrame = CGRect(x: 0, y: -4, width: 20, height: 20)
          let imageColor = UIColor.linkBlue()
          let ascentImage = NSTextAttachment()
          ascentImage.image = UIImage(resource: .icEmAscent24)
            .withRenderingMode(.alwaysTemplate)
            .withTintColor(imageColor)
          ascentImage.bounds = imageFrame
          let descentImage = NSTextAttachment()
          descentImage.image = UIImage(resource: .icEmDescent24)
            .withRenderingMode(.alwaysTemplate)
            .withTintColor(imageColor)
          descentImage.bounds = imageFrame
          ascentImageString = NSAttributedString(attachment: ascentImage)
          descentImageString = NSAttributedString(attachment: descentImage)
        } else {
          ascentImageString = NSAttributedString(string: "↗", attributes: attributes)
          descentImageString = NSAttributedString(string: "↘", attributes: attributes)
        }
        let elevation = NSMutableAttributedString(string: "")
        elevation.append(MWMNavigationDashboardEntity.estimateDot())
        elevation.append(ascentImageString)
        elevation.append(NSAttributedString(string: " \(totalAscent)  ", attributes: attributes))
        elevation.append(descentImageString)
        elevation.append(NSAttributedString(string: " \(totalDescent)", attributes: attributes))
        let elevationInfo = RoutePreview.ElevationInfo(estimates: elevation, image: image)
        self.process(.updateElevationInfo(elevationInfo))
      }
  }
}
