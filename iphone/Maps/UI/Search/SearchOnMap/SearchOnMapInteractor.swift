final class SearchOnMapInteractor: NSObject {

  private let presenter: SearchOnMapPresenter
  private let searchManager: SearchManager.Type
  private var isUpdatesDisabled = false
  private var showResultsOnMap: Bool = false

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
    case .didStartTyping:
      response = .setIsTyping(true)
    case .didType(let searchText):
      isUpdatesDisabled = false
      searchManager.searchQuery(searchText.text,
                                forInputLocale: searchText.locale,
                                withCategory: false)
      showResultsOnMap = true
      response = .startSearching
    case .clearButtonDidTap:
      isUpdatesDisabled = true
      searchManager.clear()
      response = .clearSearch
    case .didSelectText(let searchText, let isCategory):
      isUpdatesDisabled = false
      response = .selectText(searchText.text)
      searchManager.saveQuery(searchText.text,
                              forInputLocale: searchText.locale)
      searchManager.searchQuery(searchText.text,
                                forInputLocale: searchText.locale,
                                withCategory: isCategory)
      showResultsOnMap = true
    case .searchButtonDidTap(let searchText):
      searchManager.saveQuery(searchText.text,
                              forInputLocale: searchText.locale)
      showResultsOnMap = true
      response = .showOnTheMap
    case .didSelectResult(let result, let index, let searchText):
      switch result.itemType {
      case .regular:
        searchManager.saveQuery(searchText.text,
                                forInputLocale:searchText.locale)
        searchManager.showResult(at: UInt(index))
        // TODO: handle routingTooltipSearch (see MWMSearchManager) for the navigation search
        response = .setSearchScreenHidden(true)
      case .suggestion:
        response = .selectText(result.suggestion)
        searchManager.searchQuery(result.suggestion,
                                  forInputLocale: searchText.locale,
                                  withCategory: result.isPureSuggest)
      @unknown default:
        fatalError("Unsupported result type")
      }
    case .didSelectPlaceOnMap:
      response = .setSearchScreenHidden(true)
    case .didDeselectPlaceOnMap:
      response = .setSearchScreenHidden(false)
      searchManager.showViewportSearchResultsOnMap()
    case .didStartDraggingMap:
      response = .setSearchScreenCompact
    case .didUpdatePresentationStep(let step):
      response = .updatePresentationStep(step)
    case .closeSearch:
      isUpdatesDisabled = true
      searchManager.clear()
      response = .close
    }
    return response
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
    presenter.process(.showResults(results, isSearchCompleted: true))
  }

  func onSearchResultsUpdated() {
    guard !isUpdatesDisabled else { return }
    let results = searchManager.getResults()
    guard !results.isEmpty else { return }
    presenter.process(.showResults(results, isSearchCompleted: false))
  }
}
