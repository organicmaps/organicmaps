import XCTest
@testable import Organic_Maps__Debug_

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
    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, false)
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
    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
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
  var scrollViewDelegate: (any SearchOnMapScrollViewDelegate)?
  func render(_ viewModel: SearchOnMap.ViewModel) {
    self.viewModel = viewModel
  }
  func close() {
  }
  func show() {
  }
}

private class SearchManagerMock: SearchManager {
  static var observers = ListenerContainer<MWMSearchObserver>()
  static var results = SearchOnMap.SearchResults.empty {
    didSet {
      observers.forEach { observer in
        observer.onSearchCompleted?()
      }
    }
  }
  private static var _searchMode: SearchMode = .everywhere

  static func add(_ observer: any MWMSearchObserver) {
    self.observers.addListener(observer)
  }

  static func remove(_ observer: any MWMSearchObserver) {
    self.observers.removeListener(observer)
  }

  static func save(_ query: SearchQuery) {}
  static func searchQuery(_ query: SearchQuery) {}
  static func showResult(at index: UInt) {}
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
      SearchResult()
    ])
  }
}
