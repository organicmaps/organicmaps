final class SearchOnMapInteractor: NSObject {

  private let presenter: SearchOnMapPresenter
  private let searchManager: SearchManager.Type
  private let routeManager: MWMRouter.Type
  private var isUpdatesDisabled = false
  private var showResultsOnMap: Bool = false

  var routingTooltipSearch: MWMSearchManagerRoutingTooltipSearch = .none

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
    guard let response = resolve(event) else { return }
    presenter.process(response)
  }

  private func resolve(_ event: SearchOnMap.Request) -> SearchOnMap.Response? {
    var response: SearchOnMap.Response?
    switch event {
    case .viewDidLoad:
      response = .showHistoryAndCategory
    case .didStartDraggingSearch:
      response = .setIsTyping(false)
    case .didStartTyping, .openSearch:
      response = .setIsTyping(true)
    case .didType(let searchText):
      response = processTypedText(searchText)
    case .clearButtonDidTap:
      response = processCancelButtonDidTap()
    case .didSelectText(let searchText, let isCategory):
      response = processSelectedText(searchText, isCategory: isCategory)
    case .searchButtonDidTap(let searchText):
      response = processSearchButtonDidTap(searchText)
    case .didSelectResult(let result, let index, let searchText):
      response = processSelectedResult(result, index: index, searchText: searchText)
    case .didSelectPlaceOnMap, .hideSearch:
      response = .setSearchScreenHidden(true)
    case .didDeselectPlaceOnMap:
      routingTooltipSearch = .none
      response = .setSearchScreenHidden(false)
      searchManager.showViewportSearchResultsOnMap()
    case .didStartDraggingMap:
      response = .setSearchScreenCompact
    case .didUpdatePresentationStep(let step):
      response = .updatePresentationStep(step)
    case .closeSearch:
      routingTooltipSearch = .none
      isUpdatesDisabled = true
      searchManager.clear()
      response = .close
    }
    return response
  }

  private func processCancelButtonDidTap() -> SearchOnMap.Response {
    isUpdatesDisabled = true
    searchManager.clear()
    return .clearSearch
  }

  private func processSearchButtonDidTap(_ searchText: SearchOnMap.SearchText) -> SearchOnMap.Response {
    searchManager.saveQuery(searchText.text,
                            forInputLocale: searchText.locale)
    showResultsOnMap = true
    return .showOnTheMap
  }

  private func processTypedText(_ searchText: SearchOnMap.SearchText) -> SearchOnMap.Response {
    isUpdatesDisabled = false
    showResultsOnMap = true
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

  private func processSelectedResult(_ result: SearchResult, index: Int, searchText: SearchOnMap.SearchText) -> SearchOnMap.Response? {
    switch result.itemType {
    case .regular:
      searchManager.saveQuery(searchText.text,
                              forInputLocale:searchText.locale)
      switch routingTooltipSearch {
      case .none:
        searchManager.showResult(at: UInt(index))
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
      return .setSearchScreenHidden(true)
    case .suggestion:
      searchManager.searchQuery(result.suggestion,
                                forInputLocale: searchText.locale,
                                withCategory: result.isPureSuggest)
      return .selectText(result.suggestion)
    @unknown default:
      fatalError("Unsupported result type")
    }
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
