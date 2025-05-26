final class SearchOnMapPresenter {
  typealias Response = SearchOnMap.Response
  typealias ViewModel = SearchOnMap.ViewModel

  weak var view: SearchOnMapView?

  private var searchState: SearchOnMapState = .searching {
    didSet {
      guard searchState != oldValue else { return }
      didChangeState?(searchState)
    }
  }

  private var viewModel: ViewModel = .initial
  private var isRouting: Bool
  private var didChangeState: ((SearchOnMapState) -> Void)?

  init(isRouting: Bool, didChangeState: ((SearchOnMapState) -> Void)?) {
    self.isRouting = isRouting
    self.didChangeState = didChangeState
    didChangeState?(searchState)
  }

  func process(_ response: SearchOnMap.Response) {
    guard response != .none else { return }

    if response == .close {
      view?.close()
      searchState = .closed
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
    case .selectQuery(let query):
      viewModel.skipSuggestions = false
      viewModel.searchingText = query.text
      viewModel.contentState = .searching

      switch query.source {
      case .typedText, .suggestion:
        viewModel.isTyping = true
      case .category, .history, .deeplink:
        viewModel.isTyping = false
        viewModel.presentationStep = isRouting ? .hidden : .halfScreen
      @unknown default:
        fatalError("Unknown search text source")
      }
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
      if step == .hidden {
        viewModel.isTyping = false
      }
      viewModel.presentationStep = step
    case .close, .none:
      break
    }
    return viewModel
  }
}

private extension ModalPresentationStep {
  var searchState: SearchOnMapState {
    switch self {
    case .fullScreen, .halfScreen, .compact:
      return .searching
    case .hidden:
      return .hidden
    }
  }
}
