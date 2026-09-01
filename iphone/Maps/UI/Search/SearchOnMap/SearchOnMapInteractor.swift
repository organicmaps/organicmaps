final class SearchOnMapInteractor: NSObject {
  private let presenter: SearchOnMapPresenter
  private let searchManager: SearchManager.Type
  private let routePointSelector: RoutePointSelecting?
  private weak var mapViewController: MapViewController?
  private var isUpdatesDisabled = false

  init(presenter: SearchOnMapPresenter,
       searchManager: SearchManager.Type = Search.self,
       routePointSelector: RoutePointSelecting? = nil,
       mapViewController: MapViewController? = nil) {
    self.presenter = presenter
    self.searchManager = searchManager
    self.routePointSelector = routePointSelector
    self.mapViewController = mapViewController
    super.init()
    searchManager.add(self)
    if routePointSelector != nil {
      mapViewController?.add(self)
    }
  }

  deinit {
    searchManager.remove(self)
    mapViewController?.remove(self)
  }

  func handle(_ event: SearchOnMap.Request) {
    let response = resolve(event)
    presenter.process(response)
  }

  func closeForReplacement() {
    prepareToCloseSearch()
    presenter.close(notifyObservers: false)
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

    case .currentLocationButtonDidTap:
      guard routePointSelector?.selectCurrentLocation() == true else { return .none }
      searchManager.clear()
      return .close

    case .chooseOnMapButtonDidTap:
      guard let routePointSelector, routePointSelector.isActive else { return .none }
      return .showMapPointPicker(routePointSelector.title)

    case .didSelectMapPoint(let point):
      guard routePointSelector?.select(mapPoint: point) == true else {
        return .setSearchScreenHidden(false)
      }
      searchManager.clear()
      return .close

    case .didCancelMapPoint:
      return closeSearch()

    case .refreshRoutePointActions:
      return .updateRoutePointActions(routePointActions)

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
      mapViewController?.updateVisibleAreaInsets(for: self, insets: insets, updatingViewport: true)
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
    // During navigation the map follows the current position, so the viewport is not overridden;
    // the results are still drawn as marks on the map. While a route point is being picked the search
    // screen stays visible, so the viewport must still be refitted to the results.
    if !presenter.shouldHideForRouting {
      searchManager.updateViewportWithResults()
    }
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
      if let routePointSelector, routePointSelector.isActive {
        guard routePointSelector.select(searchResult: result) else { return .none }
        searchManager.clear()
        return .close
      }
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
    .setSearchScreenHidden(false)
  }

  private func closeSearch() -> SearchOnMap.Response {
    prepareToCloseSearch()
    return .close
  }

  private func prepareToCloseSearch() {
    isUpdatesDisabled = true
    searchManager.clear()
    routePointSelector?.cancel()
  }

  private var routePointActions: SearchOnMap.ViewModel.RoutePointActions? {
    guard let routePointSelector, routePointSelector.isActive else { return nil }
    return .init(title: routePointSelector.title,
                 canSelectCurrentLocation: routePointSelector.canSelectCurrentLocation)
  }

  private func searchModeForPresentationStep(_ step: SearchOnMapModalPresentationStep) -> SearchMode {
    switch step {
    case .expanded:
      return isiPad ? .everywhereAndViewport : .everywhere
    case .halfScreen, .compact:
      return .everywhereAndViewport
    case .hidden:
      return .viewport
    }
  }
}

// MARK: - LocationModeListener

extension SearchOnMapInteractor: LocationModeListener {
  func processMyPositionStateModeEvent(_: MWMMyPositionMode) {
    handle(.refreshRoutePointActions)
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
