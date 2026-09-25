@testable import Organic_Maps__Debug_
import XCTest

final class BookmarksListSortingTests: XCTestCase {
  private var view: MockBookmarksSortingView!
  private var interactor: MockBookmarksSortingInteractor!
  private var presenter: BookmarksListPresenter!
  private let cafeZ = MockSortingBookmark(id: 1, name: "Zulu cafe")
  private let museum = MockSortingBookmark(id: 2, name: "Museum")
  private let cafeA = MockSortingBookmark(id: 3, name: "Alpha cafe")

  override func setUp() {
    super.setUp()
    view = MockBookmarksSortingView()
    interactor = MockBookmarksSortingInteractor(bookmarks: [cafeZ, museum, cafeA])
    presenter = BookmarksListPresenter(view: view, router: MockBookmarksSortingRouter(), interactor: interactor)
    presenter.viewDidLoad()
  }

  override func tearDown() {
    presenter = nil
    interactor = nil
    view = nil
    super.tearDown()
  }

  func testSortingKeepsOnlyMatchingBookmarksInSortedOrder() {
    presenter.search("cafe")
    interactor.searchCompletions[0]([cafeZ, cafeA])
    presenter.sort()
    view.menu[0].action()
    interactor.sortCompletions[0]([BookmarksSection(title: "A–Z", bookmarks: [cafeA, museum, cafeZ], tracks: nil)])

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId), [.bookmark(3), .bookmark(1)])
    XCTAssertEqual(view.sections.map(\.sectionTitle), ["A–Z"])
  }

  func testResettingSortIgnoresPendingSortAndKeepsSearchRelevance() throws {
    presenter.search("cafe")
    interactor.searchCompletions[0]([cafeZ, cafeA])
    presenter.sort()
    view.menu[0].action()
    try XCTUnwrap(view.menu.last).action()
    interactor.sortCompletions[0]([BookmarksSection(title: "A–Z", bookmarks: [cafeA, museum, cafeZ], tracks: nil)])

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId), [.bookmark(1), .bookmark(3)])
  }

  func testSearchUsesSelectedSortingAndOmitsUnmatchedSectionsAndTracks() {
    presenter.sort()
    view.menu[2].action()
    interactor.sortCompletions[0]([
      BookmarksSection(title: "Cafes", bookmarks: [cafeA, cafeZ], tracks: nil),
      BookmarksSection(title: "Museums", bookmarks: [museum], tracks: nil),
      BookmarksSection(title: "Tracks", bookmarks: nil, tracks: [MockSortingTrack()]),
    ])
    presenter.search("cafe")
    interactor.searchCompletions[0]([cafeZ, cafeA])

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId), [.bookmark(3), .bookmark(1)])
    XCTAssertEqual(view.sections.map(\.sectionTitle), ["Cafes"])
  }

  func testLateSortUsesTheCurrentQueryResults() {
    presenter.search("cafe")
    interactor.searchCompletions[0]([cafeZ, cafeA])
    presenter.sort()
    view.menu[0].action()
    presenter.search("museum")
    interactor.searchCompletions[1]([museum])
    interactor.sortCompletions[0]([BookmarksSection(title: "A–Z", bookmarks: [cafeA, museum, cafeZ], tracks: nil)])

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId), [.bookmark(2)])
  }

  func testClearingSearchRestoresTheFullSortedCategory() {
    presenter.search("cafe")
    interactor.searchCompletions[0]([cafeZ, cafeA])
    presenter.sort()
    view.menu[0].action()
    let sorted = [BookmarksSection(title: "A–Z", bookmarks: [cafeA, museum, cafeZ], tracks: nil),
                  BookmarksSection(title: "Tracks", bookmarks: nil, tracks: [MockSortingTrack()])]
    interactor.sortCompletions[0](sorted)
    presenter.cancelSearch()
    interactor.sortCompletions[1](sorted)

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId),
                   [.bookmark(3), .bookmark(2), .bookmark(1), .track(99)])
  }

  func testSortingDoesNotReplaceEmptySearchResultsWithTheCategory() {
    presenter.search("no match")
    interactor.searchCompletions[0]([])
    presenter.sort()
    view.menu[0].action()
    interactor.sortCompletions[0]([BookmarksSection(title: "A–Z", bookmarks: [cafeA, museum, cafeZ], tracks: nil)])

    XCTAssertTrue(view.sections.isEmpty)
  }

  func testSortFinishingBeforeSearchStillFiltersTheResults() {
    presenter.sort()
    view.menu[0].action()
    presenter.search("cafe")
    interactor.sortCompletions[0]([BookmarksSection(title: "A–Z", bookmarks: [cafeA, museum, cafeZ], tracks: nil)])
    interactor.searchCompletions[0]([cafeZ, cafeA])

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId), [.bookmark(3), .bookmark(1)])
  }

  func testLateSearchCannotReapplyFilterAfterCancellation() {
    presenter.search("cafe")
    presenter.cancelSearch()
    interactor.searchCompletions[0]([cafeZ, cafeA])

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId),
                   [.track(99), .bookmark(1), .bookmark(2), .bookmark(3)])
  }

  func testReloadReappliesTheQueryAndSelectedSorting() {
    presenter.search("cafe")
    interactor.searchCompletions[0]([cafeZ, cafeA])
    presenter.sort()
    view.menu[0].action()
    let sorted = [BookmarksSection(title: "A–Z", bookmarks: [cafeA, museum, cafeZ], tracks: nil)]
    interactor.sortCompletions[0](sorted)
    presenter.viewDidAppear()
    // The category may have changed while the user was looking at the map.
    interactor.searchCompletions[1]([cafeZ])
    interactor.sortCompletions[1](sorted)

    XCTAssertEqual(view.sections.flatMap(\.editableItems).map(\.itemId), [.bookmark(1)])
  }
}

private final class MockSortingBookmark: Bookmark {
  private let id: MWMMarkID
  private let name: String
  init(id: MWMMarkID, name: String) {
    self.id = id
    self.name = name
    super.init()
  }

  override var bookmarkId: MWMMarkID { id }
  override var bookmarkName: String { name }
  override var bookmarkType: String? { nil }
  override var bookmarkColor: UIColor { .red }
  override var bookmarkIconName: String { "" }
  override var locationCoordinate: CLLocationCoordinate2D { CLLocationCoordinate2D(latitude: 0, longitude: 0) }
}

private final class MockSortingTrack: Track {
  override var trackId: MWMTrackID { 99 }
  override var trackName: String { "Test track" }
  override var trackLengthMeters: Int { 100 }
  override var trackColor: UIColor { .blue }
  override var isVisible: Bool { true }
}

private final class MockSortingBookmarkGroup: BookmarkGroup {
  private let items: [Bookmark]
  init(bookmarks: [Bookmark]) {
    items = bookmarks
    super.init(categoryId: 1, bookmarksManager: BookmarksManager.shared())
  }

  override var title: String { "Test" }
  override var detailedAnnotation: String { "" }
  override var hasDescription: Bool { false }
  override var isHtmlDescription: Bool { false }
  override var imageUrl: URL? { nil }
  override var bookmarks: [Bookmark] { items }
  override var tracks: [Track] { [MockSortingTrack()] }
  override var bookmarksCount: Int { items.count }
  override var trackCount: Int { 1 }
}

private final class MockBookmarksSortingInteractor: IBookmarksListInteractor {
  var onCategoryReload: ((GroupReloadingResult) -> Void)?
  var searchCompletions: [([Bookmark]) -> Void] = []
  var sortCompletions: [([BookmarksSection]) -> Void] = []
  var sortingType: BookmarksListSortingType?
  private let group: BookmarkGroup
  init(bookmarks: [Bookmark]) { group = MockSortingBookmarkGroup(bookmarks: bookmarks) }
  func getBookmarkGroup() -> BookmarkGroup { group }
  func reloadCategory() { onCategoryReload?(.success) }
  func prepareForSearch() {}
  func search(_: String, completion: @escaping ([Bookmark]) -> Void) { searchCompletions.append(completion) }
  func availableSortingTypes(hasMyPosition _: Bool) -> [BookmarksListSortingType] { [.name, .date, .type] }
  func sort(_ type: BookmarksListSortingType, location _: CLLocation?, completion: @escaping ([BookmarksSection]) -> Void) {
    sortingType = type
    sortCompletions.append(completion)
  }

  func resetSort() { sortingType = nil }
  func lastSortingType() -> BookmarksListSortingType? { sortingType }
  func viewOnMap() {}
  func viewBookmarkOnMap(_: MWMMarkID) {}
  func viewTrackOnMap(_: MWMTrackID) {}
  func setTrack(_: MWMTrackID, visible _: Bool) {}
  func deleteItems(with _: Set<BookmarksListItemId>) {}
  func moveItems(with _: Set<BookmarksListItemId>, toGroupId _: MWMMarkGroupID) {}
  func setColor(_: UIColor, for _: Set<BookmarksListItemId>) {}
  func deleteBookmarksGroup() {}
  func canDeleteGroup() -> Bool { true }
  func exportFile(fileType _: FileType, completion _: @escaping SharingResultCompletionHandler) {}
  func finishExportFile() {}
}

private final class MockBookmarksSortingView: IBookmarksListView {
  var sections: [IBookmarksListSectionViewModel] = []
  var menu: [IBookmarksListMenuItem] = []
  func setSections(_ sections: [IBookmarksListSectionViewModel]) { self.sections = sections }
  func showMenu(_ items: [IBookmarksListMenuItem], from _: BookmarkToolbarButtonSource) { menu = items }
  func saveSearchStateBeforeShowingOnMap(searchText _: String?) {}
  func setInfo(_: IBookmarksListInfoViewModel) {}
  func showColorPicker(anchor _: UIView?, currentColor _: UIColor?, _: ((UIColor) -> Void)?) {}
  func showBatchColorPicker(_: ((UIColor) -> Void)?) {}
  func finishEditing() {}
  func enableEditing(_: Bool) {}
  func share(_: URL, displayName _: String, completion _: @escaping () -> Void) {}
  func showError(title _: String, message _: String) {}
}

private final class MockBookmarksSortingRouter: IBookmarksListRouter {
  func listSettings(_: BookmarkGroup, delegate _: CategorySettingsViewControllerDelegate?) {}
  func viewOnMap(_: BookmarkGroup) {}
  func showDescription(_: BookmarkGroup) {}
  func selectGroup(currentGroupId _: MWMMarkGroupID, delegate _: SelectBookmarkGroupViewControllerDelegate?) {}
  func editBookmark(bookmarkId _: MWMMarkID, completion _: @escaping (Bool) -> Void) {}
  func editTrack(trackId _: MWMTrackID, completion _: @escaping (Bool) -> Void) {}
  func goBack() {}
}
