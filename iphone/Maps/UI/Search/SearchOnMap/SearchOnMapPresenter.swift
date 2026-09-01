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
  let shouldHideForRouting: Bool
  private var didChangeState: ((SearchOnMapState) -> Void)?

  init(shouldHideForRouting: Bool,
       routePointActions: ViewModel.RoutePointActions? = nil,
       didChangeState: ((SearchOnMapState) -> Void)?) {
    self.shouldHideForRouting = shouldHideForRouting
    self.didChangeState = didChangeState
    viewModel.routePointActions = routePointActions
    didChangeState?(searchState)
  }

  func process(_ response: SearchOnMap.Response) {
    guard response != .none else { return }

    if response == .close {
      close()
      return
    }

    if case .showMapPointPicker(let title) = response {
      viewModel.isTyping = false
      viewModel.presentationStep = .hidden
      view?.render(viewModel)
      searchState = .mapPointPicker
      view?.showMapPointPicker(title: title)
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

  func close(notifyObservers: Bool = true) {
    view?.close()
    if notifyObservers {
      searchState = .closed
    }
    didChangeState = nil
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
      viewModel.presentationStep = shouldHideForRouting ? .hidden : .compact
      if case .results(var results) = viewModel.contentState, !results.isEmpty {
        results.skipSuggestions()
        viewModel.contentState = .results(results)
      }
    case .setIsTyping(let isSearching):
      viewModel.isTyping = isSearching
      if isSearching {
        viewModel.presentationStep = .expanded
      }
    case .showHistoryAndCategory:
      viewModel.isTyping = true
      viewModel.contentState = .historyAndCategory
      viewModel.presentationStep = .expanded
    case .showResults(var searchResults, let isSearchCompleted):
      if viewModel.skipSuggestions {
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
        viewModel.presentationStep = shouldHideForRouting ? .hidden : .halfScreen
      @unknown default:
        fatalError("Unknown search text source")
      }
    case .clearSearch:
      viewModel.searchingText = ""
      viewModel.isTyping = true
      viewModel.skipSuggestions = false
      viewModel.contentState = .historyAndCategory
      viewModel.presentationStep = .expanded
    case .setSearchScreenHidden(let isHidden):
      viewModel.isTyping = false
      let visibleStep = shouldHideForRouting ? .expanded : viewModel.latestVisiblePresentationStep
      viewModel.presentationStep = isHidden ? .hidden : visibleStep
    case .setSearchScreenCompact:
      viewModel.isTyping = false
      viewModel.presentationStep = .compact
    case .updatePresentationStep(let step):
      if step == .hidden {
        viewModel.isTyping = false
      }
      viewModel.presentationStep = step
    case .updateRoutePointActions(let actions):
      viewModel.routePointActions = actions
    case .showMapPointPicker, .close, .none:
      break
    }
    return viewModel
  }
}

private extension SearchOnMapModalPresentationStep {
  var searchState: SearchOnMapState {
    switch self {
    case .expanded, .halfScreen, .compact:
      return .searching
    case .hidden:
      return .hidden
    }
  }
}
