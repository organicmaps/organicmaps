extension NavigationDashboard {
  final class Presenter: NSObject {
    private weak var view: NavigationDashboardViewController?
    private var viewModel: ViewModel = .initial
    private var latestVisiblePresentationStep: NavigationDashboardModalPresentationStep = .compact
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
        let navigationInfo = viewModel.navigationInfo.copyWith(dashboardState: state)
        viewModel = viewModel.copyWith(navigationInfo: navigationInfo,
                                       dashboardState: state)

      case .close:
        viewModel = viewModel.copyWith(dashboardState: .closed)

      case .setHidden(let hidden):
        switch hidden {
        case true:
          viewModel = viewModel.copyWith(presentationStep: .hidden)
          // Hide side buttons if it is not in navigation state
          if viewModel.dashboardState != .navigation {
            viewModel = viewModel.copyWith(dashboardState: .hidden)
          }
        case false:
          // Skip presentation step updates when the screen is presented
          if viewModel.presentationStep == .hidden {
            let step: NavigationDashboardModalPresentationStep = (hidden ? .hidden : latestVisiblePresentationStep).forNavigationState(viewModel.dashboardState)
            viewModel = viewModel.copyWith(presentationStep: step)
          }
          // Show side buttons if it is not in navigation state
          if viewModel.dashboardState != .navigation {
            viewModel = viewModel.copyWith(dashboardState: .prepare)
          }
        }
      case .updatePresentationStep(let step):
        viewModel = viewModel.copyWith(presentationStep: step.forNavigationState(viewModel.dashboardState))

      case .showNavigationDashboard:
        viewModel = viewModel.copyWith(dashboardState: .navigation,
                                       presentationStep: .hidden)

      case .updateRouteBuildingProgress(let progress, routerType: let routerType):
        viewModel = viewModel.copyWith(routerType: routerType,
                                       progress: progress)

      case .updateNavigationInfo(let entity):
        let estimates = buildEstimatesString(routerType: viewModel.routerType,
                                             navigationInfo: entity,
                                             elevationInfo: viewModel.elevationInfo)
        viewModel = viewModel.copyWith(entity: entity, estimates: estimates)

      case .updateElevationInfo(let elevationInfo):
        let estimates = buildEstimatesString(routerType: viewModel.routerType,
                                             navigationInfo: viewModel.entity,
                                             elevationInfo: elevationInfo)
        viewModel = viewModel.copyWith(elevationInfo: elevationInfo, estimates: estimates)

      case .updateNavigationInfoAvailableArea(let rect):
        let navigationInfo = viewModel.navigationInfo.copyWith(availableArea: rect)
        viewModel = viewModel.copyWith(navigationInfo: navigationInfo)

      case .updateSearchState(let state):
        isSearchOpened = state == .searching
        switch state {
        case .closed:
          viewModel = resolve(action: .setHidden(false), with: viewModel)
            .copyWith(navigationSearchState: .minimizedNormal)
        case .hidden:
          viewModel = resolve(action: .setHidden(PlacePageData.hasData), with: viewModel)
            .copyWith(navigationSearchState: .minimizedNormal)
        case .searching:
          viewModel = resolve(action: .setHidden(true), with: viewModel)
            .copyWith(navigationSearchState: .minimizedNormal)
        @unknown default:
          fatalError("Unknown search state: \(state)")
        }

      case .updateDrivingOptionsState(_):
        viewModel = viewModel.copyWith(routingOptions: RoutingOptions())

      case let .show(points, routerType):
        viewModel = viewModel.copyWith(routePoints: NavigationDashboard.RoutePoints(points: points),
                                       routerType: routerType)
        if !isSearchOpened && viewModel.presentationStep == .hidden {
          let step = latestVisiblePresentationStep.forNavigationState(viewModel.dashboardState)
          viewModel = viewModel.copyWith(presentationStep: step)
        }

      case .showError(let errorMessage):
        viewModel = viewModel.copyWith(errorMessage: errorMessage)
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
