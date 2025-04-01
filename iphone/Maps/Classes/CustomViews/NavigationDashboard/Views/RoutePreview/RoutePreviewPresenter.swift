extension RoutePreview {
  final class Presenter: NSObject {
    private weak var view: RoutePreviewViewController?
    private var viewModel: ViewModel = .initial

    init(view: RoutePreviewViewController) {
      self.view = view
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
      case .close:
        viewModel.shouldClose = true
      case .setHidden(let hidden):
        viewModel.presentationStep = hidden ? .hidden : .fullScreen
      case .updatePresentationStep(let step):
        viewModel.presentationStep = step
      case .showNavigationDashboard:
        viewModel.presentationStep = .hidden
      case .updateRouteBuildingProgress(let progress, routerType: let routerType):
        print("progress", progress)
        // TODO: Handle progress update
      case let .show(points, routerType):
        viewModel.points = points
        viewModel.routerType = routerType
        viewModel.presentationStep = .halfScreen
      }
      viewModel.isStartRoutingAllowed = viewModel.points.count > 1
      return viewModel
    }
  }
}

