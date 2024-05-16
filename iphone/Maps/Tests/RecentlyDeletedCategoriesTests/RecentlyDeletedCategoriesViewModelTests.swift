import XCTest
@testable import Organic_Maps__Debug_

final class RecentlyDeletedCategoriesViewModelTests: XCTestCase {
  var viewModel: RecentlyDeletedCategoriesViewModel!
  var bookmarksManagerMock: MockRecentlyDeletedCategoriesManager!

  override func setUp() {
    super.setUp()
    bookmarksManagerMock = MockRecentlyDeletedCategoriesManager()
    setupBookmarksManagerStubs()

    viewModel = RecentlyDeletedCategoriesViewModel(bookmarksManager: bookmarksManagerMock)
    viewModel.handleAction(.fetchRecentlyDeletedCategories)
  }

  override func tearDown() {
    viewModel = nil
    bookmarksManagerMock = nil
    super.tearDown()
  }

  private func setupBookmarksManagerStubs() {
    bookmarksManagerMock.categories = [
      RecentlyDeletedCategory(fileName: "test1", fileURL: URL(string: "test1")!, deletionDate: Date().timeIntervalSince1970),
      RecentlyDeletedCategory(fileName: "test2", fileURL: URL(string: "test2")!, deletionDate: Date().timeIntervalSince1970),
      RecentlyDeletedCategory(fileName: "lol", fileURL: URL(string: "lol")!, deletionDate: Date().timeIntervalSince1970),
      RecentlyDeletedCategory(fileName: "te1", fileURL: URL(string: "te1")!, deletionDate: Date().timeIntervalSince1970),
    ]
  }

  func testInitializationFetchesCategories() {
    XCTAssertEqual(viewModel.state.interactionMode, .normal)
    XCTAssertEqual(viewModel.state.filteredDataSource.flatMap { $0.categories }.map { $0.fileURL }, bookmarksManagerMock.getRecentlyDeletedCategories())
  }

  func testFetchRecentlyDeletedCategories() {
    let expectation = XCTestExpectation(description: "Expect updateCategories event")
    viewModel.didReceiveEvent = { event in
      if case .updateCategories = event {
        expectation.fulfill()
      }
    }

    viewModel.handleAction(.fetchRecentlyDeletedCategories)
    wait(for: [expectation], timeout: 1.0)
  }

  // MARK: - Selection Tests
  func testMultipleSelectionAndDeselection() {
    viewModel.handleAction(.selectAll)
    let initialSelectedCount = viewModel.state.selectedIndexPaths.count
    XCTAssertEqual(initialSelectedCount, viewModel.state.filteredDataSource.flatMap { $0.categories }.count)

    viewModel.handleAction(.deselectAll)
    XCTAssertTrue(viewModel.state.selectedIndexPaths.isEmpty)
  }

  func testSelectAndDeselectSpecificCategory() {
    let specificIndexPath = IndexPath(row: 0, section: 0)
    viewModel.handleAction(.select(at: specificIndexPath))
    XCTAssertTrue(viewModel.state.selectedIndexPaths.contains(specificIndexPath))

    viewModel.handleAction(.delete(at: specificIndexPath))
    XCTAssertFalse(viewModel.state.selectedIndexPaths.contains(specificIndexPath))
  }

  func testSelectAndDeselectSpecificCategories() {
    let indexPath1 = IndexPath(row: 0, section: 0)
    let indexPath2 = IndexPath(row: 1, section: 0)
    let indexPath3 = IndexPath(row: 2, section: 0)

    viewModel.handleAction(.select(at: indexPath1))
    viewModel.handleAction(.select(at: indexPath2))
    viewModel.handleAction(.select(at: indexPath3))
    XCTAssertTrue(viewModel.state.selectedIndexPaths.contains(indexPath1))
    XCTAssertTrue(viewModel.state.selectedIndexPaths.contains(indexPath2))
    XCTAssertTrue(viewModel.state.selectedIndexPaths.contains(indexPath3))

    viewModel.handleAction(.deselect(at: indexPath1))
    XCTAssertFalse(viewModel.state.selectedIndexPaths.contains(indexPath1))
    XCTAssertEqual(viewModel.state.interactionMode, .editingAndSomeSelected)

    viewModel.handleAction(.deselect(at: indexPath2))
    viewModel.handleAction(.deselect(at: indexPath3))
    XCTAssertEqual(viewModel.state.selectedIndexPaths.count, .zero)
    XCTAssertEqual(viewModel.state.interactionMode, .editingAndNothingSelected)
  }

  func testStateChangesOnSelection() {
    let indexPath = IndexPath(row: 1, section: 0)
    viewModel.handleAction(.select(at: indexPath))
    XCTAssertEqual(viewModel.state.interactionMode, .editingAndSomeSelected)

    viewModel.handleAction(.deselect(at: indexPath))
    XCTAssertEqual(viewModel.state.interactionMode, .editingAndNothingSelected)
  }

  func testStateChangesOnDone() {
    let indexPath = IndexPath(row: 1, section: 0)
    viewModel.handleAction(.select(at: indexPath))
    XCTAssertEqual(viewModel.state.interactionMode, .editingAndSomeSelected)

    viewModel.handleAction(.cancelSelecting)
    XCTAssertEqual(viewModel.state.filteredDataSource.flatMap { $0.categories }.map { $0.fileURL }, bookmarksManagerMock.getRecentlyDeletedCategories())
  }

  // MARK: - Searching Tests
  func testSearchWithEmptyString() {
    viewModel.handleAction(.search(""))
    XCTAssertEqual(viewModel.state.filteredDataSource.flatMap { $0.categories }.count, 4)
  }

  func testSearchWithNoResults() {
    viewModel.handleAction(.search("no results"))
    XCTAssertTrue(viewModel.state.filteredDataSource.allSatisfy { $0.categories.isEmpty })
  }

  func testSearchFiltering() {
    let expectation = XCTestExpectation(description: "Expect updateCategories event on search")
    viewModel.didReceiveEvent = { event in
      if case .updateCategories = event {
        expectation.fulfill()
      }
    }

    viewModel.handleAction(.search("test"))
    wait(for: [expectation], timeout: 1.0)
  }

  func testModeTransitionEvents() {
    let expectation = XCTestExpectation(description: "Expect updateInteractionMode event")
    viewModel.didReceiveEvent = { event in
      if case let .updateInteractionMode(mode) = event, mode == .searching {
        expectation.fulfill()
      }
    }

    viewModel.handleAction(.startSearching)
    wait(for: [expectation], timeout: 1.0)
  }

  func testCancelSearchRestoresDataSource() {
    let searchText = "test"
    viewModel.handleAction(.search(searchText))
    XCTAssertEqual(viewModel.state.interactionMode, .searching)
    XCTAssertTrue(viewModel.state.filteredDataSource.allSatisfy { $0.categories.allSatisfy { $0.fileName.localizedCaseInsensitiveContains(searchText) } })
    XCTAssertEqual(viewModel.state.filteredDataSource.flatMap { $0.categories }.count, 2)

    viewModel.handleAction(.cancelSearching)
    XCTAssertEqual(viewModel.state.interactionMode, .normal)
    XCTAssertEqual(viewModel.state.filteredDataSource.flatMap { $0.categories }.count, 4)
  }

  // MARK: - Deletion Tests

  func testDeleteSingleCategory() {
      let indexPath = IndexPath(row: 0, section: 0)
      let expectation = XCTestExpectation(description: "Expect removeCategory event")
      viewModel.didReceiveEvent = { event in
          if case let .removeCategory(at: indexPaths) = event, indexPaths.contains(indexPath) {
              expectation.fulfill()
          }
      }

      viewModel.handleAction(.delete(at: indexPath))
      wait(for: [expectation], timeout: 1.0)
  }


  func testDeleteAllWhenNoOneIsSelected() {
    viewModel.handleAction(.deleteSelected)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 0)
  }

  func testDeleteMultipleSelectedCategories() {
    viewModel.handleAction(.select(at: IndexPath(row: 0, section: 0)))
    viewModel.handleAction(.select(at: IndexPath(row: 1, section: 0)))

    let updateDataSourceExpectation = XCTestExpectation(description: "Expect updateCategories event for multiple")
    let updateInteractionModeExpectation = XCTestExpectation(description: "Expect updateInteractionMode event for multiple")
    var callCount = 0
    viewModel.didReceiveEvent = { event in
      switch event {
      case .updateCategories(let dataSource):
        XCTAssertEqual(dataSource.flatMap { $0.categories }.count, 2)
        XCTAssertEqual(dataSource.flatMap { $0.categories }.map { $0.fileURL }, self.bookmarksManagerMock.getRecentlyDeletedCategories())
        updateDataSourceExpectation.fulfill()
      case .updateInteractionMode(let interactionMode):
        XCTAssertEqual(interactionMode, .normal)
        updateInteractionModeExpectation.fulfill()
      default:
        XCTFail("Unexpected event")
      }
    }

    viewModel.handleAction(.deleteSelected)
    wait(for: [updateDataSourceExpectation, updateInteractionModeExpectation], timeout: 1.0)
  }

  // MARK: - Recovery Tests
  func testRecoverCategory() {
    viewModel.handleAction(.recover(at: IndexPath(row: 0, section: 0)))
    XCTAssertEqual(viewModel.state.interactionMode, .normal)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 3)
    XCTAssertEqual(viewModel.state.interactionMode, .normal)
  }

  func testRecoverAll() {
    viewModel.handleAction(.recoverSelected)
    XCTAssertEqual(viewModel.state.interactionMode, .normal)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 0)
  }

  func testRecoverAllWhenSomeAreSelected() {
    viewModel.handleAction(.select(at: IndexPath(row: 0, section: 0)))
    viewModel.handleAction(.select(at: IndexPath(row: 1, section: 0)))
    viewModel.handleAction(.recoverSelected)
    XCTAssertEqual(viewModel.state.interactionMode, .normal)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 2)
    XCTAssertEqual(viewModel.state.filteredDataSource.flatMap { $0.categories }.map { $0.fileURL }, bookmarksManagerMock.getRecentlyDeletedCategories())
  }

  func testSearchFiltersCategories() {
    var searchText = "test"
    viewModel.handleAction(.search(searchText))
    XCTAssertEqual(viewModel.state.interactionMode, .searching)
    XCTAssertTrue(viewModel.state.filteredDataSource.allSatisfy { $0.categories.allSatisfy { $0.fileName.localizedCaseInsensitiveContains(searchText) } })

    searchText = "te"
    viewModel.handleAction(.search(searchText))
    XCTAssertEqual(viewModel.state.interactionMode, .searching)
    XCTAssertTrue(viewModel.state.filteredDataSource.allSatisfy { $0.categories.allSatisfy { $0.fileName.localizedCaseInsensitiveContains(searchText) } })
  }

  func testDeleteAllCategories() {
    viewModel.handleAction(.deleteSelected)
    XCTAssertTrue(bookmarksManagerMock.categories.isEmpty)
  }

  func testRecoverAllCategories() {
    viewModel.handleAction(.recoverSelected)
    XCTAssertTrue(bookmarksManagerMock.categories.isEmpty)
  }

  func testDeleteAndRecoverAllCategoriesWhenEmpty() {
    bookmarksManagerMock.categories = []
    viewModel.handleAction(.fetchRecentlyDeletedCategories)
    viewModel.handleAction(.deselectAll)
    viewModel.handleAction(.recoverSelected)
    XCTAssertTrue(viewModel.state.filteredDataSource.isEmpty)
  }

  func testMultipleStateTransitions() {
    viewModel.handleAction(.startSelecting)
    XCTAssertEqual(viewModel.state.interactionMode, .editingAndNothingSelected)

    viewModel.handleAction(.startSearching)
    XCTAssertEqual(viewModel.state.interactionMode, .searching)

    viewModel.handleAction(.cancelSearching)
    viewModel.handleAction(.cancelSelecting)
    XCTAssertEqual(viewModel.state.interactionMode, .normal)
  }
}
