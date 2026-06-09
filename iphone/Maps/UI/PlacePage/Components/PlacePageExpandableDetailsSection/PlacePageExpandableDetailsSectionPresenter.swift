final class PlacePageExpandableDetailsSectionPresenter {
  private var viewModel: PlacePageExpandableDetailsSectionViewModel

  weak var view: PlacePageExpandableDetailsSectionViewController?

  init(viewModel: PlacePageExpandableDetailsSectionViewModel) {
    self.viewModel = viewModel
  }

  func process(_ responses: [PlacePageExpandableDetailsSectionResponse]) {
    var currentViewModel = viewModel
    for response in responses {
      currentViewModel = resolve(response, with: currentViewModel)
    }
    viewModel = currentViewModel
    view?.render(currentViewModel)
  }

  private func resolve(_ response: PlacePageExpandableDetailsSectionResponse, with previousViewModel: PlacePageExpandableDetailsSectionViewModel) -> PlacePageExpandableDetailsSectionViewModel {
    var viewModel = previousViewModel
    switch response {
    case .expandText:
      if case .collapsed = viewModel.expandedState {
        viewModel.expandedState = .expanded
      }
    case .updateTitle(let string):
      viewModel.title = string
    case .updateIcon(let image):
      viewModel.icon = image
    case .updateAccessory(let image):
      viewModel.accessory = image
    case .updateExpandableText(let string, let isHTML):
      guard let string, !string.isEmpty else {
        viewModel.expandableText = nil
        return viewModel
      }

      viewModel.expandableText = isHTML ? .html(string) : .plain(string)
      viewModel.expandedState = .collapsed
    }
    return viewModel
  }
}
