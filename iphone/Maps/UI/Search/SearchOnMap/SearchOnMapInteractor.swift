final class SearchOnMapInteractor: NSObject {

  private let presenter: SearchOnMapPresenter
  private let searchManager: SearchManager.Type
  private let routeManager: MWMRouter.Type
  private var isUpdatesDisabled = false

  var routingTooltipSearch: SearchOnMapRoutingTooltipSearch = .none

  init(presenter: SearchOnMapPresenter,
       searchManager: SearchManager.Type = Search.self,
       routeManager: MWMRouter.Type = MWMRouter.self) {
    self.presenter = presenter
    self.searchManager = searchManager
    self.routeManager = routeManager
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
      switch routingTooltipSearch {
      case .none:
        searchManager.showResult(at: result.index)
      case .start:
        let point = MWMRoutePoint(cgPoint: result.point,
                                  title: result.titleText,
                                  subtitle: result.addressText,
                                  type: .start,
                                  intermediateIndex: 0)
        routeManager.build(from: point, bestRouter: false)
      case .finish:
        let point = MWMRoutePoint(cgPoint: result.point,
                                  title: result.titleText,
                                  subtitle: result.addressText,
                                  type: .finish,
                                  intermediateIndex: 0)
        routeManager.build(to: point, bestRouter: false)
      @unknown default:
        fatalError("Unsupported routingTooltipSearch")
      }
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
    routingTooltipSearch = .none
    return .setSearchScreenHidden(false)
  }

  private func closeSearch() -> SearchOnMap.Response {
    routingTooltipSearch = .none
    isUpdatesDisabled = true
    searchManager.clear()
    return .close
  }

  private func searchModeForPresentationStep(_ step: ModalPresentationStep) -> SearchMode {
    switch step {
    case .fullScreen:
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
