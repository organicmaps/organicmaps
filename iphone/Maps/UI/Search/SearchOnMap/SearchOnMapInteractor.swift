final class SearchOnMapInteractor: NSObject {

  private let presenter: SearchOnMapPresenter
  private let searchManager: SearchManager.Type
  private var isUpdatesDisabled = false

  init(presenter: SearchOnMapPresenter,
       searchManager: SearchManager.Type = Search.self) {
    self.presenter = presenter
    self.searchManager = searchManager
    super.init()
    searchManager.add(self)
  }

  deinit {
    searchManager.remove(self)
  }

  func handle(_ event: SearchOnMap.Request) {
    let response = resolve(event)
    presenter.process(response)
  }

  private func resolve(_ event: SearchOnMap.Request) -> SearchOnMap.Response {
    switch event {
    case .openSearch:
      return .showHistoryAndCategory

    case .hideSearch:
      return .setSearchScreenHidden(true)

    case .didStartDraggingSearch:
      return .setIsTyping(false)

    case .didStartTyping:
      return .setIsTyping(true)

    case .didType(let searchText):
      return processTypedText(searchText)

    case .clearButtonDidTap:
      return processClearButtonDidTap()

    case .didSelect(let searchText):
      return processSelectedText(searchText)

    case .searchButtonDidTap(let searchText):
      return processSearchButtonDidTap(searchText)

    case .didSelectResult(let result, let query):
      return processSelectedResult(result, query: query)

    case .didSelectPlaceOnMap:
      return isiPad ? .none : .setSearchScreenHidden(true)

    case .didDeselectPlaceOnMap:
      return deselectPlaceOnMap()

    case .didStartDraggingMap:
      return .setSearchScreenCompact

    case .didUpdatePresentationStep(let step):
      searchManager.setSearchMode(searchModeForPresentationStep(step))
      return .updatePresentationStep(step)

    case .updateVisibleAreaInsets(let insets):
      MapViewController.shared()!.updateVisibleAreaInsets(for: self, insets: insets, updatingViewport: true)
      return .none
    case .closeSearch:
      return closeSearch()
    }
  }

  private func processClearButtonDidTap() -> SearchOnMap.Response {
    isUpdatesDisabled = true
    searchManager.clear()
    return .clearSearch
  }

  private func processSearchButtonDidTap(_ query: SearchQuery) -> SearchOnMap.Response {
    searchManager.save(query)
    return .showOnTheMap
  }

  private func processTypedText(_ query: SearchQuery) -> SearchOnMap.Response {
    isUpdatesDisabled = false
    searchManager.searchQuery(query)
    return .startSearching
  }

  private func processSelectedText(_ query: SearchQuery) -> SearchOnMap.Response {
    isUpdatesDisabled = false
    if query.source != .history {
      searchManager.save(query)
    }
    searchManager.searchQuery(query)
    return .selectQuery(query)
  }

  private func processSelectedResult(_ result: SearchResult, query: SearchQuery) -> SearchOnMap.Response {
    switch result.itemType {
    case .regular:
      searchManager.save(query)
      searchManager.showResult(at: result.index)
      return isiPad ? .none : .setSearchScreenHidden(true)
    case .suggestion:
      let suggestionQuery = SearchQuery(result.suggestion,
                                        locale: query.locale,
                                        source: result.isPureSuggest ? .suggestion : .typedText)
      searchManager.searchQuery(suggestionQuery)
      return .selectQuery(suggestionQuery)
    @unknown default:
      fatalError("Unsupported result type")
    }
  }

  private func deselectPlaceOnMap() -> SearchOnMap.Response {
    return .setSearchScreenHidden(false)
  }

  private func closeSearch() -> SearchOnMap.Response {
    isUpdatesDisabled = true
    searchManager.clear()
    return .close
  }

  private func searchModeForPresentationStep(_ step: SearchOnMapModalPresentationStep) -> SearchMode {
    switch step {
    case .expanded:
      return isiPad ? .everywhereAndViewport : .everywhere
    case .halfScreen, .compact:
      return  .everywhereAndViewport
    case .hidden:
      return  .viewport
    }
  }
}

// MARK: - MWMSearchObserver
extension SearchOnMapInteractor: MWMSearchObserver {
  func onSearchCompleted() {
    guard !isUpdatesDisabled, searchManager.searchMode() != .viewport else { return }
    let results = searchManager.getResults()
    presenter.process(.showResults(SearchOnMap.SearchResults(results), isSearchCompleted: true))
  }

  func onSearchResultsUpdated() {
    guard !isUpdatesDisabled, searchManager.searchMode() != .viewport else { return }
    let results = searchManager.getResults()
    guard !results.isEmpty else { return }
    presenter.process(.showResults(SearchOnMap.SearchResults(results), isSearchCompleted: false))
  }
}
