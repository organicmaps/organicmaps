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
  }

  override func tearDown() {
    viewModel = nil
    bookmarksManagerMock = nil
    super.tearDown()
  }

  private func setupBookmarksManagerStubs() {
    bookmarksManagerMock.categories = [
      RecentlyDeletedCategory(title: "test1", fileURL: URL(string: "test1")!, deletionDate: Date()),
      RecentlyDeletedCategory(title: "test2", fileURL: URL(string: "test2")!, deletionDate: Date()),
      RecentlyDeletedCategory(title: "lol", fileURL: URL(string: "lol")!, deletionDate: Date()),
      RecentlyDeletedCategory(title: "te1", fileURL: URL(string: "te1")!, deletionDate: Date()),
    ]
  }

  func testInitializationFetchesCategories() {
    XCTAssertEqual(viewModel.state, .nothingSelected)
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, Int(bookmarksManagerMock.recentlyDeletedCategoriesCount()))
  }

  // MARK: - Selection Tests
  func testMultipleSelectionAndDeselection() {
    viewModel.selectAllCategories()
    let initialSelectedCount = viewModel.selectedIndexPaths.count
    XCTAssertEqual(initialSelectedCount, viewModel.filteredDataSource.flatMap { $0.content }.count)

    viewModel.deselectAllCategories()
    XCTAssertTrue(viewModel.selectedIndexPaths.isEmpty)
  }

  func testSelectAndDeselectSpecificCategory() {
    let specificIndexPath = IndexPath(row: 0, section: 0)
    viewModel.selectCategory(at: specificIndexPath)
    XCTAssertTrue(viewModel.selectedIndexPaths.contains(specificIndexPath))

    viewModel.deselectCategory(at: specificIndexPath)
    XCTAssertFalse(viewModel.selectedIndexPaths.contains(specificIndexPath))
  }

  func testSelectAndDeselectSpecificCategories() {
    let indexPath1 = IndexPath(row: 0, section: 0)
    let indexPath2 = IndexPath(row: 1, section: 0)
    let indexPath3 = IndexPath(row: 2, section: 0)
    viewModel.selectCategory(at: indexPath1)
    viewModel.selectCategory(at: indexPath2)
    viewModel.selectCategory(at: indexPath3)
    XCTAssertTrue(viewModel.selectedIndexPaths.contains(indexPath1))
    XCTAssertTrue(viewModel.selectedIndexPaths.contains(indexPath2))
    XCTAssertTrue(viewModel.selectedIndexPaths.contains(indexPath3))

    viewModel.deselectCategory(at: indexPath1)
    XCTAssertFalse(viewModel.selectedIndexPaths.contains(indexPath1))
    XCTAssertEqual(viewModel.state, .someSelected)

    viewModel.deselectCategory(at: indexPath2)
    viewModel.deselectCategory(at: indexPath3)
    XCTAssertEqual(viewModel.selectedIndexPaths.count, .zero)
    XCTAssertEqual(viewModel.state, .nothingSelected)
  }

  func testStateChangesOnSelection() {
    let indexPath = IndexPath(row: 1, section: 0)
    viewModel.selectCategory(at: indexPath)
    XCTAssertEqual(viewModel.state, .someSelected)

    viewModel.deselectCategory(at: indexPath)
    XCTAssertEqual(viewModel.state, .nothingSelected)
  }

  func testStateChangesOnDone() {
    let indexPath = IndexPath(row: 1, section: 0)
    viewModel.selectCategory(at: indexPath)
    XCTAssertEqual(viewModel.state, .someSelected)

    viewModel.cancelSelecting()
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, Int(bookmarksManagerMock.recentlyDeletedCategoriesCount()))
  }

  // MARK: - Searching Tests
  func testSearchWithEmptyString() {
    viewModel.search("")
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, 4)
  }

  func testSearchWithNoResults() {
    viewModel.search("xyz") // Assuming "xyz" matches no category names
    XCTAssertTrue(viewModel.filteredDataSource.allSatisfy { $0.content.isEmpty })
  }

  func testCancelSearchRestoresDataSource() {
    let searchText = "test"
    viewModel.search(searchText)
    XCTAssertEqual(viewModel.state, .searching)
    XCTAssertTrue(viewModel.filteredDataSource.allSatisfy { $0.content.allSatisfy { $0.title.localizedCaseInsensitiveContains(searchText) } })
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, 2)

    viewModel.cancelSearching()
    XCTAssertEqual(viewModel.state, .nothingSelected)
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, 4)
  }

  // MARK: - Deletion Tests
  func testDeleteCategory() {
    let initialCount = bookmarksManagerMock.categories.count
    viewModel.deleteCategory(at: IndexPath(row: 0, section: 0))
    XCTAssertEqual(bookmarksManagerMock.categories.count, initialCount - 1)
  }

  func testDeleteAllWhenNoOneIsSelected() {
    viewModel.deleteSelectedCategories()
    XCTAssertEqual(bookmarksManagerMock.categories.count, .zero)
  }

  func testDeleteAllWhenNoSoneAreSelected() {
    viewModel.selectCategory(at: IndexPath(row: 0, section: 0))
    viewModel.selectCategory(at: IndexPath(row: 1, section: 0))
    viewModel.deleteSelectedCategories()
    XCTAssertEqual(viewModel.state, .nothingSelected)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 2)
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, 2)
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, Int(bookmarksManagerMock.recentlyDeletedCategoriesCount()))
  }

  // MARK: - Recovery Tests
  func testRecoverCategory() {
    viewModel.recoverCategory(at: IndexPath(row: 0, section: 0))
    XCTAssertEqual(viewModel.state, .nothingSelected)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 3)
    XCTAssertEqual(viewModel.state, .nothingSelected)
  }

  func testRecoverAll() {
    viewModel.recoverSelectedCategories()
    XCTAssertEqual(viewModel.state, .nothingSelected)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 0)
  }

  func testRecoverAllWhenSomeAreSelected() {
    viewModel.selectCategory(at: IndexPath(row: 0, section: 0))
    viewModel.selectCategory(at: IndexPath(row: 1, section: 0))
    viewModel.recoverSelectedCategories()
    XCTAssertEqual(viewModel.state, .nothingSelected)
    XCTAssertEqual(bookmarksManagerMock.categories.count, 2)
    XCTAssertEqual(viewModel.filteredDataSource.flatMap { $0.content }.count, Int(bookmarksManagerMock.recentlyDeletedCategoriesCount()))
  }

  func testSearchFiltersCategories() {
    var searchText = "test"
    viewModel.search(searchText)
    XCTAssertEqual(viewModel.state, .searching)
    XCTAssertTrue(viewModel.filteredDataSource.allSatisfy { $0.content.allSatisfy { $0.title.localizedCaseInsensitiveContains(searchText) } })

    searchText = "te"
    viewModel.search(searchText)
    XCTAssertEqual(viewModel.state, .searching)
    XCTAssertTrue(viewModel.filteredDataSource.allSatisfy { $0.content.allSatisfy { $0.title.localizedCaseInsensitiveContains(searchText) } })
  }

  func testDeleteAllCategories() {
    viewModel.deleteSelectedCategories()
    XCTAssertTrue(bookmarksManagerMock.categories.isEmpty)
  }

  func testRecoverAllCategories() {
    viewModel.recoverSelectedCategories()
    XCTAssertTrue(bookmarksManagerMock.categories.isEmpty)
  }

  func testDeleteAndRecoverAllCategoriesWhenEmpty() {
    bookmarksManagerMock.categories = []
    viewModel.fetchRecentlyDeletedCategories()
    viewModel.deleteSelectedCategories()
    viewModel.recoverSelectedCategories()
    XCTAssertTrue(viewModel.filteredDataSource.isEmpty)
  }

  func testMultipleStateTransitions() {
    viewModel.startSelecting()
    XCTAssertEqual(viewModel.state, .nothingSelected)

    viewModel.startSearching()
    XCTAssertEqual(viewModel.state, .searching)

    viewModel.cancelSearching()
    viewModel.cancelSelecting()
    XCTAssertEqual(viewModel.state, .nothingSelected)
  }
}
