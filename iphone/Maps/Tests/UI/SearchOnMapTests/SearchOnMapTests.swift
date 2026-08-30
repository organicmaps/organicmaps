@testable import Organic_Maps__Debug_
import XCTest

final class SearchOnMapTests: XCTestCase {
  private var presenter: SearchOnMapPresenter!
  private var interactor: SearchOnMapInteractor!
  private var view: SearchOnMapViewMock!
  private var searchManager: SearchManagerMock.Type!
  private var currentState: SearchOnMapState = .searching

  override func setUp() {
    super.setUp()
    searchManager = SearchManagerMock.self
    presenter = SearchOnMapPresenter(isRouting: false,
                                     didChangeState: { [weak self] in self?.currentState = $0 })
    interactor = SearchOnMapInteractor(presenter: presenter, searchManager: searchManager)
    view = SearchOnMapViewMock()
    presenter.view = view
  }

  override func tearDown() {
    presenter = nil
    interactor = nil
    view = nil
    searchManager.results = .empty
    searchManager.setSearchMode(.everywhere)
    searchManager.fitViewportCallsCount = 0
    searchManager = nil
    super.tearDown()
  }

  func test_GivenViewIsLoading_WhenViewLoads_ThenShowsHistoryAndCategory() {
    interactor.handle(.openSearch)

    XCTAssertEqual(currentState, .searching)
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    XCTAssertEqual(view.viewModel.contentState, .historyAndCategory)
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)
  }

  func test_GivenInitialState_WhenSelectCategory_ThenUpdateSearchResultsAndShowMap() {
    interactor.handle(.openSearch)

    let query = SearchQuery("category", source: .category)
    interactor.handle(.didSelect(query))

    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
    XCTAssertEqual(view.viewModel.contentState, .searching)
    XCTAssertEqual(view.viewModel.searchingText, query.text)
    XCTAssertEqual(view.viewModel.isTyping, false)

    let results = SearchResult.stubResults()
    searchManager.results = results

    XCTAssertEqual(currentState, .searching)
    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, false)
  }

  func test_GivenInitialState_WhenTypeText_ThenUpdateSearchResults() {
    interactor.handle(.openSearch)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didType(query))

    XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    XCTAssertEqual(view.viewModel.contentState, .searching)
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)

    let results = SearchResult.stubResults()
    searchManager.results = results

    XCTAssertEqual(currentState, .searching)
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)
  }

  func test_GivenInitialState_WhenTapSearch_ThenUpdateSearchResultsAndShowMap() {
    interactor.handle(.openSearch)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didType(query))

    let results = SearchResult.stubResults()
    searchManager.results = results

    XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)

    interactor.handle(.searchButtonDidTap(query))

    XCTAssertEqual(currentState, .searching)
    XCTAssertEqual(view.viewModel.presentationStep, .compact)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, false)
  }

  func test_GivenHalfScreenSearch_WhenTapSearch_ThenShowMapInCompact() {
    interactor.handle(.openSearch)
    interactor.handle(.didUpdatePresentationStep(.halfScreen))
    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.searchButtonDidTap(query))
    interactor.handle(.didUpdatePresentationStep(view.viewModel.presentationStep))

    XCTAssertEqual(view.viewModel.presentationStep, .compact)
    XCTAssertEqual(view.viewModel.isTyping, false)
    XCTAssertEqual(searchManager.searchMode(), .everywhereAndViewport)
  }

  func test_GivenResults_WhenTapSearch_ThenFitViewport() {
    interactor.handle(.openSearch)
    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didType(query))
    searchManager.results = SearchResult.stubResults()

    interactor.handle(.searchButtonDidTap(query))
    XCTAssertEqual(searchManager.fitViewportCallsCount, 1)
  }

  func test_GivenNoResultsYet_WhenTapSearch_ThenFitViewportOnceTheyArrive() {
    interactor.handle(.openSearch)
    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didType(query))

    // The search restarts on every keystroke, so submitting it right away finds no results yet.
    interactor.handle(.searchButtonDidTap(query))
    XCTAssertEqual(searchManager.fitViewportCallsCount, 0)

    searchManager.results = SearchResult.stubResults()
    XCTAssertEqual(searchManager.fitViewportCallsCount, 1)

    // The request is one-shot: later updates of the same search must not move the map again.
    searchManager.results = SearchResult.stubResults()
    XCTAssertEqual(searchManager.fitViewportCallsCount, 1)
  }

  func test_GivenPendingFit_WhenTypeAgain_ThenDoNotFitViewport() {
    interactor.handle(.openSearch)
    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didType(query))
    interactor.handle(.searchButtonDidTap(query))

    interactor.handle(.didType(SearchQuery("text2", source: .typedText)))
    searchManager.results = SearchResult.stubResults()
    XCTAssertEqual(searchManager.fitViewportCallsCount, 0)
  }

  func test_GivenRouting_WhenTapSearch_ThenHideSearch() {
    presenter = SearchOnMapPresenter(isRouting: true,
                                     didChangeState: { [weak self] in self?.currentState = $0 })
    interactor = SearchOnMapInteractor(presenter: presenter, searchManager: searchManager)
    presenter.view = view
    interactor.handle(.openSearch)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didType(query))
    interactor.handle(.searchButtonDidTap(query))

    // Skipping the fit during navigation is the core's decision, see Framework::FitSearchResults().
    XCTAssertEqual(view.viewModel.presentationStep, .hidden)
  }

  func test_GivenSearchIsOpened_WhenMapIsDragged_ThenCollapseSearchScreen() {
    interactor.handle(.openSearch)
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)

    interactor.handle(.didStartDraggingMap)
    XCTAssertEqual(view.viewModel.presentationStep, .compact)
  }

  func test_GivenSearchIsOpened_WhenModalPresentationScreenIsDragged_ThenDisableTyping() {
    interactor.handle(.openSearch)
    XCTAssertEqual(view.viewModel.isTyping, true)

    interactor.handle(.didStartDraggingSearch)
    XCTAssertEqual(view.viewModel.isTyping, false)
  }

  func test_GivenResultsOnScreen_WhenSelectResult_ThenHideSearch() {
    interactor.handle(.openSearch)
    XCTAssertEqual(view.viewModel.isTyping, true)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didSelect(query))

    let results = SearchResult.stubResults()
    searchManager.results = results

    interactor.handle(.didSelectResult(results[0], withQuery: query))
    if isiPad {
      XCTAssertEqual(currentState, .searching)
      XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    } else {
      XCTAssertEqual(currentState, .hidden)
      XCTAssertEqual(view.viewModel.presentationStep, .hidden)
    }
  }

  func test_GivenSearchIsActive_WhenSelectPlaceOnMap_ThenHideSearch() {
    interactor.handle(.openSearch)
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)

    interactor.handle(.didSelectPlaceOnMap)

    if isiPad {
      XCTAssertNotEqual(view.viewModel.presentationStep, .hidden)
    } else {
      XCTAssertEqual(view.viewModel.presentationStep, .hidden)
    }
  }

  func test_GivenSearchIsHidden_WhenPPDeselected_ThenShowSearch() {
    interactor.handle(.openSearch)
    XCTAssertEqual(view.viewModel.isTyping, true)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didSelect(query))

    let results = SearchResult.stubResults()
    searchManager.results = results

    interactor.handle(.didSelectResult(results[0], withQuery: query))
    if isiPad {
      XCTAssertEqual(currentState, .searching)
      XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    } else {
      XCTAssertEqual(currentState, .hidden)
      XCTAssertEqual(view.viewModel.presentationStep, .hidden)
    }

    interactor.handle(.didDeselectPlaceOnMap)
    XCTAssertEqual(currentState, .searching)
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)
  }

  func test_GivenSearchIsOpen_WhenCloseSearch_ThenHideSearch() {
    interactor.handle(.openSearch)
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)

    interactor.handle(.closeSearch)
    XCTAssertEqual(currentState, .closed)
  }

  func test_GivenSearchHasText_WhenClearSearch_ThenShowHistoryAndCategory() {
    interactor.handle(.openSearch)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didSelect(query))

    interactor.handle(.clearButtonDidTap)
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    XCTAssertEqual(view.viewModel.contentState, .historyAndCategory)
    XCTAssertEqual(view.viewModel.searchingText, "")
    XCTAssertEqual(view.viewModel.isTyping, true)
  }

  func test_GivenSearchExecuted_WhenNoResults_ThenShowNoResults() {
    interactor.handle(.openSearch)

    let query = SearchQuery("text", source: .typedText)
    interactor.handle(.didSelect(query))

    searchManager.results = SearchOnMap.SearchResults([])
    interactor.onSearchCompleted()

    XCTAssertEqual(view.viewModel.contentState, .noResults)
  }

  func test_GivenSearchIsActive_WhenSelectSuggestion_ThenReplaceWithSuggestion() {
    interactor.handle(.openSearch)

    let query = SearchQuery("ca", source: .typedText)
    interactor.handle(.didType(query))

    let result = SearchResult(titleText: "", type: .suggestion, suggestion: "cafe")
    interactor.handle(.didSelectResult(result, withQuery: query))

    XCTAssertEqual(view.viewModel.searchingText, "cafe")
    XCTAssertEqual(view.viewModel.presentationStep, .expanded)
    XCTAssertEqual(view.viewModel.contentState, .searching)
    XCTAssertEqual(view.viewModel.isTyping, true)
  }

  func test_GivenSearchIsActive_WhenPasteDeeplink_ThenShowResult() {
    interactor.handle(.openSearch)

    let query = SearchQuery("om://search?cll=42.0,44.0&query=Toilet", source: .deeplink)
    interactor.handle(.didSelect(query))

    let result = SearchResult(titleText: "some result", type: .regular, suggestion: "")
    let results = SearchOnMap.SearchResults([result])
    searchManager.results = results
    interactor.onSearchCompleted()

    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
    XCTAssertEqual(view.viewModel.isTyping, false) // No typing when deeplink is used
  }

  func test_GivenSearchIsActive_WhenPresentationStepUpdate_ThenUpdateSearchMode() {
    interactor.handle(.openSearch)
    XCTAssertEqual(searchManager.searchMode(), isiPad ? .everywhereAndViewport : .everywhere)

    interactor.handle(.didUpdatePresentationStep(.halfScreen))
    XCTAssertEqual(searchManager.searchMode(), .everywhereAndViewport)

    interactor.handle(.didUpdatePresentationStep(.compact))
    XCTAssertEqual(searchManager.searchMode(), .everywhereAndViewport)

    interactor.handle(.didUpdatePresentationStep(.hidden))
    XCTAssertEqual(searchManager.searchMode(), .viewport)

    interactor.handle(.didUpdatePresentationStep(.expanded))
    XCTAssertEqual(searchManager.searchMode(), isiPad ? .everywhereAndViewport : .everywhere)
  }
}

// MARK: - Mocks

private class SearchOnMapViewMock: SearchOnMapView {
  var viewModel: SearchOnMap.ViewModel = .initial
  var scrollViewDelegate: (any UIScrollViewDelegate)?
  func render(_ viewModel: SearchOnMap.ViewModel) {
    self.viewModel = viewModel
  }

  func close() {}

  func show() {}
}

private class SearchManagerMock: SearchManager {
  static var observers = ListenerContainer<MWMSearchObserver>()
  static var results = SearchOnMap.SearchResults.empty {
    didSet {
      // MWMSearch notifies about every batch first and completes when all the searches are done.
      observers.forEach { $0.onSearchResultsUpdated?() }
      observers.forEach { $0.onSearchCompleted?() }
    }
  }

  private static var _searchMode: SearchMode = .everywhere
  static var fitViewportCallsCount = 0

  static func add(_ observer: any MWMSearchObserver) {
    observers.addListener(observer)
  }

  static func remove(_ observer: any MWMSearchObserver) {
    observers.removeListener(observer)
  }

  static func save(_: SearchQuery) {}
  static func searchQuery(_: SearchQuery) {}
  static func showResult(at _: UInt) {}
  static func fitViewportToResults() { fitViewportCallsCount += 1 }
  static func clear() {}
  static func getResults() -> [SearchResult] { results.results }
  static func searchMode() -> SearchMode { _searchMode }
  static func setSearchMode(_ mode: SearchMode) { _searchMode = mode }
}

private extension SearchResult {
  static func stubResults() -> SearchOnMap.SearchResults {
    SearchOnMap.SearchResults([
      SearchResult(),
      SearchResult(),
      SearchResult(),
    ])
  }
}
