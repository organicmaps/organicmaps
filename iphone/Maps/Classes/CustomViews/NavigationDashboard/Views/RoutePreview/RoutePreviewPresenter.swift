extension RoutePreview {
  final class Presenter: NSObject {
    private weak var view: RoutePreviewViewController?
    private var viewModel: ViewModel = .initial
    private let placePageManagerHelper: MWMPlacePageManagerHelper.Type

    init(view: RoutePreviewViewController,
         placePageManagerHelper: MWMPlacePageManagerHelper.Type = MWMPlacePageManagerHelper.self) {
      self.view = view
      self.placePageManagerHelper = placePageManagerHelper
      super.init()
    }

    func process(_ response: Response) {
      let newViewModel = resolve(action: response, with: viewModel)
      if viewModel != newViewModel {
        viewModel = newViewModel
        view?.render(newViewModel)
      }
    }

    private func resolve(action: Response, with previousViewModel: ViewModel) -> ViewModel {
      var viewModel = previousViewModel
      switch action {
      case .none:
        break

      case .prepare:
        viewModel = viewModel.copy(state: .prepare)

      case .goBack:
        viewModel = viewModel.copy(shouldClose: true)
        placePageManagerHelper.recoverPlacePage()

      case .close:
        viewModel = viewModel.copy(shouldClose: true)

      case .setHidden(let hidden):
        if hidden {
          viewModel = viewModel.copy(presentationStep: .hidden)
        } else {
          viewModel = viewModel.copy(presentationStep: .halfScreen.fromNavigationState(viewModel.dashboardState))
        }

      case .updatePresentationStep(let step):
        viewModel = viewModel.copy(presentationStep: step.fromNavigationState(viewModel.dashboardState))

      case .showNavigationDashboard:
        viewModel = viewModel.copy(state: .navigation,
                                   presentationStep: .hidden)

      case .updateRouteBuildingProgress(let progress, routerType: let routerType):
        viewModel = viewModel.copy(progress: progress)
        viewModel = viewModel.copy(routerType: routerType)

      case .updateNavigationInfo(let entity):
        let estimates = buildEstimatesString(routerType: viewModel.routerType,
                                             navigationInfo: entity,
                                             elevationInfo: viewModel.elevationInfo)
        viewModel = viewModel.copy(entity: entity, estimates: estimates)

      case .updateElevationInfo(let elevationInfo):
        let estimates = buildEstimatesString(routerType: viewModel.routerType,
                                             navigationInfo: viewModel.entity,
                                             elevationInfo: elevationInfo)
        viewModel = viewModel.copy(elevationInfo: elevationInfo, estimates: estimates)

      case .updateNavigationInfoAvailableArea(let rect):
        let navigationInfo = viewModel.navigationInfo.copy(availableArea: rect)
        viewModel = viewModel.copy(navigationInfo: navigationInfo)

      case .updateSearchState(let state):
        // TODO: 1 start route, 2 search and select category, 3 select PP, 4 deselect PP - the search will appear - it is a bug because during the navigation sth search should not be shown on the pp dissapear
        switch state {
        case .closed:
          viewModel = resolve(action: .setHidden(false), with: viewModel)
            .copy(navigationSearchState: .minimizedNormal)
        case .hidden, .searching:
          viewModel = resolve(action: .setHidden(true), with: viewModel)
            .copy(navigationSearchState: .minimizedSearch)
        @unknown default:
          fatalError("Unknown search state: \(state)")
        }

      case let .show(points, routerType):
        viewModel = viewModel.copy(routePoints: RoutePreview.RoutePoints(points: points))
        viewModel = viewModel.copy(routerType: routerType)
        if viewModel.presentationStep == .hidden {
          viewModel = viewModel.copy(presentationStep: .halfScreen.fromNavigationState(viewModel.dashboardState))
        }
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


private extension ModalPresentationStep {
  func fromNavigationState(_ state: MWMNavigationDashboardState) -> Self {
    guard state != .navigation else {
      return .hidden
    }
    return self
  }
}
