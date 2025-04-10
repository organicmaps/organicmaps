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
      case .goBack:
        viewModel.shouldClose = true
        placePageManagerHelper.recoverPlacePage()
      case .close:
        viewModel.shouldClose = true
      case .setHidden(let hidden):
        viewModel.presentationStep = hidden ? .hidden : .halfScreen
      case .updatePresentationStep(let step):
        viewModel.presentationStep = step
      case .showNavigationDashboard:
        viewModel.presentationStep = .hidden
      case .updateRouteBuildingProgress(let progress, routerType: let routerType):
        viewModel.progress = progress
        viewModel.routerType = routerType
      case .updateNavigationInfo(let entity):
        viewModel.entity = entity
        // TODO: build elevation info
        if let estimates = viewModel.entity.estimate().mutableCopy() as? NSMutableAttributedString {
//          if let elevation = self.elevation {
//            result.append(MWMNavigationDashboardEntity.estimateDot())
//            result.append(elevation)
//          }
          viewModel.estimates = estimates
        }
      case let .show(points, routerType):
        print("update route points")
        viewModel.routePoints = RoutePreview.RoutePoints(points: points)
        viewModel.routerType = routerType
        viewModel.presentationStep = .halfScreen
      }
      return viewModel
    }
  }
}

