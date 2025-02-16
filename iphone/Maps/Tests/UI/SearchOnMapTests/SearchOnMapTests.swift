import XCTest
@testable import Organic_Maps__Debug_

final class SearchOnMapTests: XCTestCase {

  private var presenter: SearchOnMapPresenter!
  private var interactor: SearchOnMapInteractor!
  private var view: SearchOnMapViewMock!
  private var searchManager: SearchManagerMock.Type!

  override func setUp() {
    super.setUp()
    searchManager = SearchManagerMock.self
    presenter = SearchOnMapPresenter(transitionManager: SearchOnMapModalTransitionManager())
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
    interactor.handle(.viewDidLoad)

    XCTAssertEqual(view.viewModel.presentationStep, .fullScreen)
    XCTAssertEqual(view.viewModel.contentState, .historyAndCategory)
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)
  }

  func test_GivenInitialState_WhenSelectCategory_ThenUpdateSearchResultsAndShowMap() {
    interactor.handle(.viewDidLoad)

    let searchText = SearchOnMap.SearchText("category")
    interactor.handle(.didSelectText(searchText, isCategory: true))

    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
    XCTAssertEqual(view.viewModel.contentState, .searching)
    XCTAssertEqual(view.viewModel.searchingText, searchText.text)
    XCTAssertEqual(view.viewModel.isTyping, false)

    let results = SearchResult.stubResults()
    searchManager.results = results

    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, false)
  }

  func test_GivenInitialState_WhenTypeText_ThenUpdateSearchResults() {
    interactor.handle(.viewDidLoad)

    let searchText = SearchOnMap.SearchText("text")
    interactor.handle(.didType(searchText))

    XCTAssertEqual(view.viewModel.presentationStep, .fullScreen)
    XCTAssertEqual(view.viewModel.contentState, .searching)
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)

    let results = SearchResult.stubResults()
    searchManager.results = results

    XCTAssertEqual(view.viewModel.presentationStep, .fullScreen)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)
  }

  func test_GivenInitialState_WhenTapSearch_ThenUpdateSearchResultsAndShowMap() {
    interactor.handle(.viewDidLoad)

    let searchText = SearchOnMap.SearchText("text")
    interactor.handle(.didType(searchText))

    let results = SearchResult.stubResults()
    searchManager.results = results

    XCTAssertEqual(view.viewModel.presentationStep, .fullScreen)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, true)

    interactor.handle(.searchButtonDidTap(searchText))

    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
    XCTAssertEqual(view.viewModel.contentState, .results(results))
    XCTAssertEqual(view.viewModel.searchingText, nil)
    XCTAssertEqual(view.viewModel.isTyping, false)
  }

  func test_GivenSearchIsOpened_WhenMapIsDragged_ThenCollapseSearchScreen() {
    interactor.handle(.viewDidLoad)
    XCTAssertEqual(view.viewModel.presentationStep, .fullScreen)

    interactor.handle(.didStartDraggingMap)
    XCTAssertEqual(view.viewModel.presentationStep, .compact)
  }

  func test_GivenSearchIsOpened_WhenModalPresentationScreenIsDragged_ThenDisableTyping() {
    interactor.handle(.viewDidLoad)
    XCTAssertEqual(view.viewModel.isTyping, true)

    interactor.handle(.didStartDraggingSearch)
    XCTAssertEqual(view.viewModel.isTyping, false)
  }

  func test_GivenResultsOnScreen_WhenSelectResult_ThenHideSearch() {
    interactor.handle(.viewDidLoad)
    XCTAssertEqual(view.viewModel.isTyping, true)

    let searchText = SearchOnMap.SearchText("text")
    interactor.handle(.didType(searchText))

    let results = SearchResult.stubResults()
    searchManager.results = results

    interactor.handle(.didSelectResult(results[0], atIndex: 0, withSearchText: searchText))
    XCTAssertEqual(view.viewModel.presentationStep, .hidden)
  }

  func test_GivenSearchIsHidden_WhenPPDeselected_ThenShowSearch() {
    interactor.handle(.viewDidLoad)
    XCTAssertEqual(view.viewModel.isTyping, true)

    let searchText = SearchOnMap.SearchText("text")
    interactor.handle(.didType(searchText))

    let results = SearchResult.stubResults()
    searchManager.results = results

    interactor.handle(.didSelectResult(results[0], atIndex: 0, withSearchText: searchText))
    XCTAssertEqual(view.viewModel.presentationStep, .hidden)

    interactor.handle(.didDeselectPlaceOnMap)
    XCTAssertEqual(view.viewModel.presentationStep, .halfScreen)
  }
}

// MARK: - Mocks

private class SearchOnMapViewMock: SearchOnMapView {
  var viewModel: SearchOnMap.ViewModel = .initial
  var scrollViewDelegate: (any SearchOnMapScrollViewDelegate)?
  func render(_ viewModel: SearchOnMap.ViewModel) {
    self.viewModel = viewModel
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

  static func add(_ observer: any MWMSearchObserver) {
    self.observers.addListener(observer)
  }
  
  static func remove(_ observer: any MWMSearchObserver) {
    self.observers.removeListener(observer)
  }
  
  static func saveQuery(_ query: String, forInputLocale inputLocale: String) {}
  static func searchQuery(_ query: String, forInputLocale inputLocale: String, withCategory isCategory: Bool) {}
  static func showResult(at index: UInt) {}
  static func showEverywhereSearchResultsOnMap() {}
  static func showViewportSearchResultsOnMap() {}
  static func clear() {}
  static func getResults() -> [SearchResult] { results.results }
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
