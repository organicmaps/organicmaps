final class SearchOnMapInteractor: NSObject {

  private let presenter: SearchOnMapPresenter
  private let searchManager: SearchManager.Type
  private let routeManager: MWMRouter.Type
  private var isUpdatesDisabled = false
  private var showResultsOnMap: Bool = false

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
    case .didSelectText(let searchText, let isCategory):
      return processSelectedText(searchText, isCategory: isCategory)
    case .searchButtonDidTap(let searchText):
      return processSearchButtonDidTap(searchText)
    case .didSelectResult(let result, let searchText):
      return processSelectedResult(result, searchText: searchText)
    case .didSelectPlaceOnMap:
      return isIPad ? .none : .setSearchScreenHidden(true)
    case .didDeselectPlaceOnMap:
      return deselectPlaceOnMap()
    case .didStartDraggingMap:
      return .setSearchScreenCompact
    case .didUpdatePresentationStep(let step):
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

  private func processSearchButtonDidTap(_ searchText: SearchOnMap.SearchText) -> SearchOnMap.Response {
    searchManager.saveQuery(searchText.text,
                            forInputLocale: searchText.locale)
    showResultsOnMap = true
    searchManager.showEverywhereSearchResultsOnMap()
    return .showOnTheMap
  }

  private func processTypedText(_ searchText: SearchOnMap.SearchText) -> SearchOnMap.Response {
    isUpdatesDisabled = false
    searchManager.searchQuery(searchText.text,
                              forInputLocale: searchText.locale,
                              withCategory: false)
    return .startSearching
  }

  private func processSelectedText(_ searchText: SearchOnMap.SearchText, isCategory: Bool) -> SearchOnMap.Response {
    isUpdatesDisabled = false
    searchManager.saveQuery(searchText.text,
                            forInputLocale: searchText.locale)
    searchManager.searchQuery(searchText.text,
                              forInputLocale: searchText.locale,
                              withCategory: isCategory)
    showResultsOnMap = true
    return .selectText(searchText.text)
  }

  private func processSelectedResult(_ result: SearchResult, searchText: SearchOnMap.SearchText) -> SearchOnMap.Response {
    switch result.itemType {
    case .regular:
      searchManager.saveQuery(searchText.text,
                              forInputLocale:searchText.locale)
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
      return isIPad ? .none : .setSearchScreenHidden(true)
    case .suggestion:
      searchManager.searchQuery(result.suggestion,
                                forInputLocale: searchText.locale,
                                withCategory: result.isPureSuggest)
      return .selectText(result.suggestion)
    @unknown default:
      fatalError("Unsupported result type")
    }
  }

  private func deselectPlaceOnMap() -> SearchOnMap.Response {
    routingTooltipSearch = .none
    searchManager.showViewportSearchResultsOnMap()
    return .setSearchScreenHidden(false)
  }

  private func closeSearch() -> SearchOnMap.Response {
    routingTooltipSearch = .none
    isUpdatesDisabled = true
    showResultsOnMap = false
    searchManager.clear()
    return .close
  }
}

// MARK: - MWMSearchObserver
extension SearchOnMapInteractor: MWMSearchObserver {
  func onSearchCompleted() {
    guard !isUpdatesDisabled else { return }
    let results = searchManager.getResults()
    if showResultsOnMap && !results.isEmpty {
      searchManager.showEverywhereSearchResultsOnMap()
      showResultsOnMap = false
    }
    presenter.process(.showResults(SearchOnMap.SearchResults(results), isSearchCompleted: true))
  }

  func onSearchResultsUpdated() {
    guard !isUpdatesDisabled else { return }
    let results = searchManager.getResults()
    guard !results.isEmpty else { return }
    presenter.process(.showResults(SearchOnMap.SearchResults(results), isSearchCompleted: false))
  }
}
