extension NavigationDashboard {
  final class Presenter: NSObject {
    private weak var view: NavigationDashboardViewController?
    private var viewModel: ViewModel = .initial
    private var latestVisiblePresentationStep: NavigationDashboardModalPresentationStep = isiPad ? .regular : .compact
    private var isSearchOpened: Bool = false
    private let placePageManagerHelper: MWMPlacePageManagerHelper.Type

    init(view: NavigationDashboardViewController,
         placePageManagerHelper: MWMPlacePageManagerHelper.Type = MWMPlacePageManagerHelper.self) {
      self.view = view
      self.placePageManagerHelper = placePageManagerHelper
      super.init()
    }

    func process(_ response: Response) {
      guard viewModel.dashboardState != .closed else { return }
      let newViewModel = resolve(action: response, with: viewModel)
      if viewModel != newViewModel {
        if newViewModel.presentationStep != .hidden {
          latestVisiblePresentationStep = newViewModel.presentationStep
        }
        viewModel = newViewModel
        view?.render(newViewModel)
      }
    }

    private func resolve(action: Response, with previousViewModel: ViewModel) -> ViewModel {
      var viewModel = previousViewModel
      switch action {
      case .none:
        break

      case .updateState(let state):
        viewModel.dashboardState = state
        viewModel.navigationInfo.state = state.navigationInfo

      case .close:
        viewModel.dashboardState = .closed

      case .setHidden(let hidden):
        switch hidden {
        case true:
          viewModel.presentationStep = .hidden
          // Hide side buttons if it is not in navigation state
          if viewModel.dashboardState != .navigation {
            viewModel.dashboardState = .hidden
          }
        case false:
          // Skip presentation step updates when the screen is presented
          if viewModel.presentationStep == .hidden {
            let step: NavigationDashboardModalPresentationStep = (hidden ? .hidden : latestVisiblePresentationStep).forNavigationState(viewModel.dashboardState)
            viewModel.presentationStep = step
          }
          // Show side buttons if it is not in navigation state
          if viewModel.dashboardState != .navigation {
            viewModel.dashboardState = .prepare
          }
        }
      case .updatePresentationStep(let step):
        viewModel.presentationStep = step.forNavigationState(viewModel.dashboardState)

      case .showNavigationDashboard:
        viewModel.dashboardState = .navigation
        viewModel.presentationStep = .hidden

      case .updateRouteBuildingProgress(let progress, routerType: let routerType):
        viewModel.routerType = routerType
        viewModel.progress = progress

      case .updateNavigationInfo(let entity):
        let estimates = buildEstimatesString(routerType: viewModel.routerType,
                                             navigationInfo: entity,
                                             elevationInfo: viewModel.elevationInfo)
        viewModel.entity = entity
        viewModel.estimates = estimates

      case .updateElevationInfo(let elevationInfo):
        let estimates = buildEstimatesString(routerType: viewModel.routerType,
                                             navigationInfo: viewModel.entity,
                                             elevationInfo: elevationInfo)
        viewModel.elevationInfo = elevationInfo
        viewModel.estimates = estimates

      case .updateNavigationInfoAvailableArea(let rect):
        viewModel.navigationInfo.availableArea = rect

      case .updateSearchState(let state):
        isSearchOpened = state == .searching
        switch state {
        case .closed:
          viewModel = resolve(action: .setHidden(false), with: viewModel)
          viewModel.navigationSearchState = .minimizedNormal
        case .hidden:
          viewModel = resolve(action: .setHidden(PlacePageData.hasData), with: viewModel)
          viewModel.navigationSearchState = .minimizedNormal
        case .searching:
          viewModel = resolve(action: .setHidden(true), with: viewModel)
          viewModel.navigationSearchState = .minimizedNormal
        @unknown default:
          fatalError("Unknown search state: \(state)")
        }

      case .updateDrivingOptionsState(let routingOptions):
        viewModel.routingOptions = routingOptions

      case let .show(points, routerType):
        var canSaveRouteAsTrack = points.count > 1
        if !viewModel.canSaveRouteAsTrack { // track was saved in the previous session
          canSaveRouteAsTrack = points.count > 1 && (viewModel.routePoints.points != points || viewModel.routerType != routerType)
        }
        viewModel.routePoints = RoutePoints(points: points)
        viewModel.routerType = routerType
        viewModel.canSaveRouteAsTrack = canSaveRouteAsTrack
        if !isSearchOpened && viewModel.presentationStep == .hidden {
          let step = latestVisiblePresentationStep.forNavigationState(viewModel.dashboardState)
          viewModel.presentationStep = step
        }

      case .setRouteAsTrackSaved:
        viewModel.canSaveRouteAsTrack = false

      case .showError(let errorMessage):
        viewModel.errorMessage = errorMessage
      }
      return viewModel
    }

    private func buildEstimatesString(routerType: MWMRouterType,
                                      navigationInfo: MWMNavigationDashboardEntity,
                                      elevationInfo: ElevationInfo?) -> NSAttributedString {
      let result = NSMutableAttributedString()
      if let estimates = navigationInfo.estimate() {
        if routerType == .ruler {
          result.append(NSAttributedString(string: L("placepage_distance") + ": "))
        }
        result.append(estimates)
      }
      if let elevationInfo {
        result.append(elevationInfo.estimates)
      }
      return result
    }
  }
}

private extension NavigationDashboardModalPresentationStep {
  func forNavigationState(_ state: MWMNavigationDashboardState) -> Self {
    guard state != .navigation else {
      return .hidden
    }
    return self
  }
}

private extension MWMNavigationDashboardState {
  var navigationInfo: MWMNavigationInfoViewState {
    switch self {
    case .navigation: return .navigation
    default: return .hidden
    }
  }
}
