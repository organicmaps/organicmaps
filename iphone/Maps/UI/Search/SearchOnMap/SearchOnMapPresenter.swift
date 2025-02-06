final class SearchOnMapPresenter {
  typealias Response = SearchOnMap.Response
  typealias ViewModel = SearchOnMap.ViewModel

  weak var view: SearchOnMapView?
  weak var presentationView: SearchOnMapModalPresentationView? { transitionManager.presentationController }
  private let transitionManager: SearchOnMapModalTransitionManager
  private var viewModel: ViewModel?

  init(transitionManager: SearchOnMapModalTransitionManager) {
    self.transitionManager = transitionManager
  }

  func process(_ action: SearchOnMap.Response) {
    guard action != .close else {
      presentationView?.close()
      return
    }
    
    if viewModel?.presentationStep == .hidden && action != .setSearchScreenHidden(false) {
      return
    }

    let newViewModel = resolve(action: action, with: viewModel ?? .initial)
    if viewModel != newViewModel {
      viewModel = newViewModel
      view?.render(newViewModel)
      presentationView?.setPresentationStep(newViewModel.presentationStep)
    }
  }

  private func resolve(action: Response, with previousViewModel: ViewModel) -> ViewModel {
    var viewModel = previousViewModel
    viewModel.searchingText = nil // should not be nil only when the text is passed to the search field

    switch action {
    case .startSearching:
      viewModel.contentState = .searching
    case .showOnTheMap:
      viewModel.isTyping = false
      viewModel.presentationStep = .halfScreen
    case .setIsTyping(let isSearching):
      viewModel.isTyping = isSearching
      if isSearching {
        viewModel.presentationStep = .fullScreen
      }
    case .showHistoryAndCategory:
      viewModel.isTyping = true
      viewModel.contentState = .historyAndCategory
      viewModel.presentationStep = .fullScreen
    case .showResults(let searchResults, let isSearchCompleted):
      viewModel.contentState = searchResults.isEmpty && isSearchCompleted ? .noResults : .results(searchResults)
    case .selectText(let text):
      viewModel.isTyping = false
      viewModel.searchingText = text
      viewModel.contentState = .searching
      viewModel.presentationStep = .halfScreen
    case .clearSearch:
      viewModel.searchingText = ""
      viewModel.isTyping = true
      viewModel.contentState = .historyAndCategory
      viewModel.presentationStep = .fullScreen
    case .setSearchScreenHidden(let isHidden):
      viewModel.isTyping = false
      viewModel.presentationStep = isHidden ? .hidden : .halfScreen
    case .setSearchScreenCompact:
      viewModel.isTyping = false
      viewModel.presentationStep = .compact
    case .updatePresentationStep(let step):
      viewModel.presentationStep = step
    case .close:
      break
    }
    return viewModel
  }
}

extension SearchOnMap.ViewModel {
  static let initial = SearchOnMap.ViewModel(isTyping: true,
                                             searchingText: nil,
                                             contentState: .historyAndCategory,
                                             presentationStep: .fullScreen)
}
