import CarPlay
@testable import Organic_Maps__Debug_
import XCTest

final class CarPlayServiceTests: XCTestCase {
  var carPlayService: CarPlayService!

  override func setUp() {
    super.setUp()
    carPlayService = CarPlayService()
  }

  override func tearDown() {
    carPlayService = nil
    // The search engine is a process-wide singleton; reset its query and mode for the next test.
    Search.clear()
    Search.setSearchMode(.everywhere)
    super.tearDown()
  }

  func testCreateEstimates() {
    let routeInfo = RouteInfo(timeToTarget: 100,
                              targetDistance: 25.2,
                              targetUnitsIndex: 1, // km
                              distanceToTurn: 0.5,
                              turnUnitsIndex: 0, // m
                              currentStreetName: "Bahdanoviča Street",
                              streetName: "Niamiha",
                              nextStreetName: "Internacyjanalnaja Street",
                              turnDirection: .left,
                              nextTurnDirection: .right,
                              turnImageName: nil,
                              nextTurnImageName: nil,
                              speedMps: 40.5,
                              speedLimitMps: 60,
                              roundExitNumber: 0,
                              isLeftHandTraffic: false)
    let estimates = carPlayService.createEstimates(routeInfo: routeInfo)

    guard let estimates else {
      XCTFail("Estimates should not be nil.")
      return
    }

    XCTAssertEqual(estimates.distanceRemaining, Measurement<UnitLength>(value: 25.2, unit: .kilometers))
    XCTAssertEqual(estimates.timeRemaining, 100)
  }

  func testStaleBookmarkSelectionCompletesOnce() throws {
    let category = try importBookmarkFixture()
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    let item = try XCTUnwrap(template.sections.first?.items.first as? CPListItem)
    let info = try XCTUnwrap(item.userInfo as? ListItemInfo)
    let metadata = try XCTUnwrap(info.metadata as? BookmarkInfo)
    let bookmarkManager = BookmarksManager.shared()
    XCTAssertTrue(bookmarkManager.hasBookmark(metadata.bookmarkId))

    bookmarkManager.deleteCategory(category.categoryId)
    XCTAssertFalse(bookmarkManager.hasBookmark(metadata.bookmarkId))
    var completionCount = 0

    carPlayService.handleListItemSelection(item) { completionCount += 1 }

    XCTAssertEqual(completionCount, 1)
  }

  func testStaleCategorySelectionCompletesOnce() throws {
    let category = try importBookmarkFixture()
    let item = try categoryListItem(for: category)
    let bookmarkManager = BookmarksManager.shared()
    XCTAssertTrue(bookmarkManager.hasCategory(category.categoryId))

    bookmarkManager.deleteCategory(category.categoryId)
    XCTAssertFalse(bookmarkManager.hasCategory(category.categoryId))
    var completionCount = 0

    carPlayService.handleListItemSelection(item) { completionCount += 1 }

    XCTAssertEqual(completionCount, 1)
  }

  func testLiveBookmarkSelectionCompletesOnce() throws {
    let category = try importBookmarkFixture()
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    let item = try XCTUnwrap(template.sections.first?.items.first as? CPListItem)
    var completionCount = 0

    carPlayService.handleListItemSelection(item) { completionCount += 1 }

    XCTAssertEqual(completionCount, 1)
  }

  func testLiveCategorySelectionReadsTheCategoryBeforeCompleting() throws {
    let fixture = try importBookmarkFixture()
    let category = BookmarkGroupSpy(categoryId: fixture.categoryId, bookmarksManager: BookmarksManager.shared())
    let item = try categoryListItem(for: fixture)
    item.userInfo = ListItemInfo(type: CPConstants.ListItemType.bookmarkLists,
                                 metadata: CategoryInfo(category: category))
    var completionCount = 0

    carPlayService.handleListItemSelection(item) {
      completionCount += 1
      XCTAssertEqual(category.titleReadCount, 1)
    }

    XCTAssertEqual(completionCount, 1)
  }

  func testBookmarkSnapshotsRespectTheDisplayLimit() throws {
    let category = try importBookmarkFixture(bookmarkCount: 3)
    let manager = BookmarksManager.shared()

    XCTAssertEqual(manager.bookmarks(forCategory: category.categoryId, limit: 2).count, 2)
    XCTAssertTrue(manager.bookmarks(forCategory: category.categoryId, limit: 0).isEmpty)
  }

  func testDeletedBookmarkListIsDetected() throws {
    let category = try importBookmarkFixture()
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    XCTAssertFalse(ListTemplateBuilder.isDeletedBookmarkList(template))

    XCTAssertTrue(BookmarksManager.shared().deleteCategory(category.categoryId))

    XCTAssertTrue(ListTemplateBuilder.isDeletedBookmarkList(template))
    ListTemplateBuilder.refreshBookmarks(in: template)
    XCTAssertTrue(template.sections.flatMap(\.items).isEmpty)
  }

  private func importBookmarkFixture(bookmarkCount: Int = 1) throws -> BookmarkGroup {
    let bookmarkManager = BookmarksManager.shared()
    let name = "CarPlayTests-\(UUID().uuidString)"
    let url = FileManager.default.temporaryDirectory.appendingPathComponent(name).appendingPathExtension("kml")
    let placemarks = (0 ..< bookmarkCount).map { index in
      """
      <Placemark>
        <name>CarPlay bookmark \(index)</name>
        <Point><coordinates>0,0</coordinates></Point>
      </Placemark>
      """
    }.joined(separator: "\n")
    let kml = """
    <?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2">
      <Document>
        <name>\(name)</name>
        \(placemarks)
      </Document>
    </kml>
    """
    try kml.write(to: url, atomically: true, encoding: .utf8)
    addTeardownBlock { try FileManager.default.removeItem(at: url) }

    let imported = expectation(description: "Bookmark fixture imported")
    let observer = BookmarksLoadObserver()
    observer.onFinished = { imported.fulfill() }
    bookmarkManager.add(observer)
    defer { bookmarkManager.remove(observer) }
    bookmarkManager.loadBookmarkFile(url)
    wait(for: [imported], timeout: 5)

    let category = try XCTUnwrap(bookmarkManager.sortedUserCategories().first { $0.title == name })
    let categoryId = category.categoryId
    addTeardownBlock {
      if bookmarkManager.hasCategory(categoryId) {
        bookmarkManager.deleteCategory(categoryId)
      }
      let trashedURLs = bookmarkManager.getRecentlyDeletedCategories()
        .filter { $0.fileURL.lastPathComponent.hasPrefix(name) }
        .map(\.fileURL)
      bookmarkManager.deleteRecentlyDeletedCategory(at: trashedURLs)
    }
    return category
  }

  private func categoryListItem(for category: BookmarkGroup) throws -> CPListItem {
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    return try XCTUnwrap(template.sections.flatMap(\.items).compactMap { $0 as? CPListItem }.first { item in
      guard let info = item.userInfo as? ListItemInfo,
            let metadata = info.metadata as? CategoryInfo else { return false }
      return metadata.category.categoryId == category.categoryId
    })
  }

  private final class BookmarksLoadObserver: NSObject, BookmarksObserver {
    var onFinished: (() -> Void)?

    func onBookmarksLoadFinished() { onFinished?() }
  }

  private final class BookmarkGroupSpy: BookmarkGroup {
    var titleReadCount = 0

    override var title: String {
      titleReadCount += 1
      return super.title
    }
  }

  func testEmptySearchCompletesImmediately() {
    let searchService = CarPlaySearchService()
    var completionCount = 0

    searchService.searchText("", forInputLocale: "en") { results in
      XCTAssertEqual(results?.isEmpty, true)
      completionCount += 1
    }

    XCTAssertEqual(completionCount, 1)
  }

  func testSupersededSearchIsCompletedWithoutResults() {
    let searchService = CarPlaySearchService()
    var completionCount = 0

    searchService.searchText("query", forInputLocale: "en") { results in
      XCTAssertNil(results, "A superseded request must be distinguishable from an empty result set.")
      completionCount += 1
    }
    searchService.searchText("", forInputLocale: "en") { _ in }

    XCTAssertEqual(completionCount, 1, "A superseded request must still be completed.")
  }

  /// MWMSearch is shared with the phone UI: its newer query cancels a CarPlay request and completes with
  /// results of its own.
  func testSearchReplacedByAnotherQueryIsCompletedWithoutResults() {
    let searchService = CarPlaySearchService()
    var completionCount = 0

    searchService.searchText("query", forInputLocale: "en") { results in
      XCTAssertNil(results, "Results of another query must not be reported.")
      completionCount += 1
    }
    Search.searchQuery(SearchQuery("another query", source: .typedText))
    (searchService as? MWMSearchObserver)?.onSearchCompleted?()

    XCTAssertEqual(completionCount, 1)
  }

  /// `MWMSearch` reports completion only for everywhere-searches, so a CarPlay request must switch
  /// the shared mode; the phone UI leaves it at viewport, where the handler would never run.
  func testSearchSwitchesSharedModeToEverywhere() {
    Search.setSearchMode(.viewport)
    let searchService = CarPlaySearchService()

    searchService.searchText("query", forInputLocale: "en") { _ in }

    XCTAssertEqual(Search.searchMode(), .everywhere)
  }

  /// A debug command starts no search, even when it reports results. Counting it as a running search would
  /// either leave it running forever or finish a search that never started, and completion of every later
  /// search, CarPlay requests included, depends on that count.
  func testDebugCommandsAreNotCountedAsRunningSearches() {
    let observer = SearchObserverSpy()
    Search.add(observer)
    defer { Search.remove(observer) }

    var reportedBatches = 0
    let statusReported = expectation(description: "The status command reports its result with an end marker")
    observer.onResultsUpdated = {
      guard Search.resultsCount() > 0 else { return }
      reportedBatches += 1
      if reportedBatches == 2 {
        statusReported.fulfill()
      }
    }

    // Reports the map download server twice, the second time with an end marker.
    XCTAssertFalse(Search.searchQuery(SearchQuery("?map-download-server", source: .typedText)))
    wait(for: [statusReported], timeout: 5)

    XCTAssertEqual(observer.startedCount, 0)
    XCTAssertEqual(observer.completedCount, 0)
  }

  /// Nothing reports the completion of a debug command, which starts no search, so CarPlay completes it at once.
  func testDebugCommandSearchCompletesImmediately() {
    let searchService = CarPlaySearchService()
    var completionCount = 0

    searchService.searchText("?map-download-server", forInputLocale: "en") { results in
      XCTAssertEqual(results?.isEmpty, true, "A debug command is not a superseded request.")
      completionCount += 1
    }

    XCTAssertEqual(completionCount, 1)
  }

  private final class SearchObserverSpy: NSObject, MWMSearchObserver {
    var startedCount = 0
    var completedCount = 0
    var onResultsUpdated: (() -> Void)?

    func onSearchStarted() { startedCount += 1 }
    func onSearchCompleted() { completedCount += 1 }
    func onSearchResultsUpdated() { onResultsUpdated?() }
  }

  func testEveryMapButtonHasAFocusedImage() {
    let template = CPMapTemplate()

    func assertMapButtonsAreComplete(_ context: String) {
      XCTAssertFalse(template.mapButtons.isEmpty, context)
      for button in template.mapButtons {
        XCTAssertNotNil(button.image, context)
        XCTAssertNotNil(button.focusedImage, context)
      }
    }

    // `.follow` and `.notFollow` between them produce all four `MapButtonType` values.
    for positionMode in [MWMMyPositionMode.follow, .notFollow] {
      MapTemplateBuilder.setupMapButtons(template, positionMode: positionMode)
      assertMapButtonsAreComplete("position mode \(positionMode.rawValue)")
    }
    MapTemplateBuilder.configurePanUI(template)
    assertMapButtonsAreComplete("panning interface")
  }

  /// CarPlay sends a focused image only in the variant it is given, so it must be resolved for the car's traits.
  func testFocusedImageFollowsTheCarTraits() {
    /// The traits of a CarPlay window, whose idiom the universal artwork must still match.
    func carTraits(_ style: UIUserInterfaceStyle) -> UITraitCollection {
      UITraitCollection(traitsFrom: [.current,
                                     UITraitCollection(userInterfaceIdiom: .carPlay),
                                     UITraitCollection(userInterfaceStyle: style)])
    }
    let light = MapTemplateBuilder.resolveFocusedImage(.btnCarplayZoomInFocused, for: carTraits(.light))
    let dark = MapTemplateBuilder.resolveFocusedImage(.btnCarplayZoomInFocused, for: carTraits(.dark))

    XCTAssertEqual(light.traitCollection.userInterfaceStyle, .light)
    XCTAssertEqual(dark.traitCollection.userInterfaceStyle, .dark)
    XCTAssertNotEqual(light.pngData(), dark.pngData())
    XCTAssertEqual(dark.renderingMode, .alwaysOriginal)
  }

  /// A pan button moves the viewport, so the map moves the opposite way.
  /// FrameworkHelper.moveMap uses an upward-positive vertical axis, unlike UIKit.
  func testPanDirectionOffset() {
    let step: CGFloat = 0.25
    // Direction, and the expected offset in `step` units.
    let expected: [(CPMapTemplate.PanDirection, CGFloat, CGFloat)] = [
      ([], 0, 0),
      ([.left], 1, 0),
      ([.right], -1, 0),
      ([.up], 0, -1),
      ([.down], 0, 1),
      ([.left, .right], 0, 0),
      ([.up, .down], 0, 0),
      ([.left, .up], 1, -1),
      ([.left, .down], 1, 1),
      ([.right, .up], -1, -1),
      ([.right, .down], -1, 1),
      ([.left, .right, .up], 0, -1),
      ([.left, .right, .down], 0, 1),
      ([.left, .up, .down], 1, 0),
      ([.right, .up, .down], -1, 0),
      ([.left, .right, .up, .down], 0, 0),
    ]

    for (direction, horizontal, vertical) in expected {
      XCTAssertEqual(direction.offset(step: step),
                     UIOffset(horizontal: horizontal * step, vertical: vertical * step),
                     "direction \(direction.rawValue)")
    }
  }

  func testListTemplateKeepsTheTypeItWasBuiltFor() {
    let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: []))

    guard let type = template.userInfo as? ListTemplateBuilder.ListTemplateType,
          case .searchResults = type
    else {
      XCTFail("The template should keep the type it was built for.")
      return
    }
  }

  func testRefreshKeepsRowsOfTemplatesThatDoNotShowBookmarks() {
    let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: []))
    template.updateSections([CPListSection(items: [CPListItem(text: "Result", detailText: nil)])])

    ListTemplateBuilder.refreshBookmarks(in: template)

    XCTAssertEqual(template.sections.first?.items.count, 1)
  }

  func testRefreshKeepsRowsOfTemplatesBuiltByAnybodyElse() {
    let section = CPListSection(items: [CPListItem(text: "Row", detailText: nil)])
    let template = CPListTemplate(title: "Any", sections: [section])

    ListTemplateBuilder.refreshBookmarks(in: template)

    XCTAssertEqual(template.sections.first?.items.count, 1)
  }
}
