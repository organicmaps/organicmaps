final class SearchOnMapPresenter {
  typealias Response = SearchOnMap.Response
  typealias ViewModel = SearchOnMap.ViewModel

  weak var view: SearchOnMapView?
  weak var presentationView: SearchOnMapModalPresentationView? { transitionManager.presentationController }

  private var searchState: SearchOnMapState = .searching {
    didSet {
      guard searchState != oldValue else { return }
      didChangeState?(searchState)
    }
  }

  private let transitionManager: SearchOnMapModalTransitionManager
  private var viewModel: ViewModel = .initial
  private var isRouting: Bool
  private var didChangeState: ((SearchOnMapState) -> Void)?

  init(transitionManager: SearchOnMapModalTransitionManager, isRouting: Bool, didChangeState: ((SearchOnMapState) -> Void)?) {
    self.transitionManager = transitionManager
    self.isRouting = isRouting
    self.didChangeState = didChangeState
    didChangeState?(searchState)
  }

  func process(_ response: SearchOnMap.Response) {
    guard response != .none else { return }

    if response == .close {
      searchState = .closed
      presentationView?.close()
      return
    }

    let showSearch = response == .setSearchScreenHidden(false) || response == .showHistoryAndCategory
    guard viewModel.presentationStep != .hidden || showSearch else {
      return
    }

    let newViewModel = resolve(action: response, with: viewModel)
    if viewModel != newViewModel {
      viewModel = newViewModel
      view?.render(newViewModel)
      searchState = newViewModel.presentationStep.searchState
      presentationView?.setPresentationStep(newViewModel.presentationStep)
    }
  }

  private func resolve(action: Response, with previousViewModel: ViewModel) -> ViewModel {
    var viewModel = previousViewModel
    viewModel.searchingText = nil // should not be nil only when the text is passed to the search field

    switch action {
    case .startSearching:
      viewModel.isTyping = true
      viewModel.skipSuggestions = false
      viewModel.contentState = .searching
    case .showOnTheMap:
      viewModel.isTyping = false
      viewModel.skipSuggestions = true
      viewModel.presentationStep = isRouting ? .hidden : .halfScreen
      if case .results(var results) = viewModel.contentState, !results.isEmpty {
        results.skipSuggestions()
        viewModel.contentState = .results(results)
      }
    case .setIsTyping(let isSearching):
      viewModel.isTyping = isSearching
      if isSearching {
        viewModel.presentationStep = .fullScreen
      }
    case .showHistoryAndCategory:
      viewModel.isTyping = true
      viewModel.contentState = .historyAndCategory
      viewModel.presentationStep = .fullScreen
    case .showResults(var searchResults, let isSearchCompleted):
      if (viewModel.skipSuggestions) {
        searchResults.skipSuggestions()
      }
      viewModel.contentState = searchResults.isEmpty && isSearchCompleted ? .noResults : .results(searchResults)
    case .selectText(let text):
      viewModel.isTyping = false
      viewModel.skipSuggestions = false
      viewModel.searchingText = text
      viewModel.contentState = .searching
      viewModel.presentationStep = isRouting ? .hidden : .halfScreen
    case .clearSearch:
      viewModel.searchingText = ""
      viewModel.isTyping = true
      viewModel.skipSuggestions = false
      viewModel.contentState = .historyAndCategory
      viewModel.presentationStep = .fullScreen
    case .setSearchScreenHidden(let isHidden):
      viewModel.isTyping = false
      viewModel.presentationStep = isHidden ? .hidden : (isRouting ? .fullScreen : .halfScreen)
    case .setSearchScreenCompact:
      viewModel.isTyping = false
      viewModel.presentationStep = .compact
    case .updatePresentationStep(let step):
      viewModel.presentationStep = step
    case .close, .none:
      break
    }
    return viewModel
  }
}

private extension ModalScreenPresentationStep {
  var searchState: SearchOnMapState {
    switch self {
    case .fullScreen, .halfScreen, .compact:
      return .searching
    case .hidden:
      return .hidden
    }
  }
}
