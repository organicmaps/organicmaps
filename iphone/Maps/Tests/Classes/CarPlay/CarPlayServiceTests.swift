import CarPlay
@testable import Organic_Maps__Debug_
import XCTest

final class CarPlayServiceTests: XCTestCase {
  var carPlayService: CarPlayService!
  private var bookmarkListObserver: CarPlayBookmarkListObserver?

  override func setUp() {
    super.setUp()
    carPlayService = CarPlayService()
  }

  override func tearDown() {
    bookmarkListObserver?.invalidate()
    bookmarkListObserver = nil
    carPlayService = nil
    // The search engine is a process-wide singleton; reset its query and mode for the next test.
    Search.clear()
    Search.setSearchMode(.everywhere)
    super.tearDown()
  }

  func testRouteLinkRetiresCarPreviewFromEitherDisplay() throws {
    try prepareRouteTransferTest()
    for display in [CarPlayMapDisplay.mainCarPlay, .dashboardCarPlay] {
      let (router, template, trip) = prepareCarPreview(on: display)

      carPlayService.prepareForRouteOnPhone()

      XCTAssertNil(router.previewTrip)
      XCTAssertNil(carPlayService.previewTemplate)
      XCTAssertTrue(carPlayService.preparedToPreviewTrips.isEmpty)
      XCTAssertEqual(carPlayService.routingOwner, .device)
      XCTAssertTrue(carPlayService.mapState.isPhoneSelected)
      XCTAssertFalse(MWMRouter.isRoutingActive(), "Transferring a preview must not rebuild it")

      seedPhoneRoute()
      carPlayService.templateWillDisappear(template, animated: false)
      carPlayService.mapTemplate(template, selectedPreviewFor: trip, using: trip.routeChoices[0])
      carPlayService.mapTemplate(template, startedTrip: trip, using: trip.routeChoices[0])

      XCTAssertEqual(MWMRouter.points().map(\.title), ["Phone start", "Phone finish"])
      XCTAssertFalse(MWMRouter.isRoutingActive())
    }
  }

  func testRouteLinkWaitsForPhoneSceneWithoutLosingNewItinerary() throws {
    try prepareRouteTransferTest()
    _ = prepareCarPreview(on: .mainCarPlay)

    carPlayService.prepareForRouteOnPhone()
    seedPhoneRoute()
    XCTAssertTrue(carPlayService.isCarplayActivated)
    XCTAssertEqual(carPlayService.routingOwner, .device)

    carPlayService.phoneSceneDidConnect()

    XCTAssertFalse(carPlayService.isCarplayActivated)
    XCTAssertEqual(carPlayService.mapState.attachedDisplay, .device)
    XCTAssertEqual(MWMRouter.points().map(\.title), ["Phone start", "Phone finish"])
    XCTAssertFalse(MWMRouter.isRoutingActive())
  }

  private func prepareRouteTransferTest() throws {
    try XCTUnwrap(MapViewController.shared()).loadViewIfNeeded()
    let previousRouterType = MWMRouter.type()
    MWMRouter.stopRouting()
    MWMRouter.setType(.ruler)
    addTeardownBlock {
      MWMRouter.stopRouting()
      MWMRouter.setType(previousRouterType)
    }
  }

  private func prepareCarPreview(on display: CarPlayMapDisplay) -> (CarPlayRouter, CPMapTemplate, CPTrip) {
    let router = CarPlayRouter()
    let start = MWMRoutePoint(cgPoint: CGPoint(x: 10, y: 10), title: "Car start", subtitle: nil, type: .start, intermediateIndex: 0)
    let finish = MWMRoutePoint(cgPoint: CGPoint(x: 12, y: 12), title: "Car finish", subtitle: nil, type: .finish, intermediateIndex: 0)
    MWMRouter.removePoints()
    MWMRouter.addPoint(start)
    MWMRouter.addPoint(finish)
    let trip = router.createTrip(startPoint: start, endPoint: finish)
    let template = MapTemplateBuilder.buildTripPreviewTemplate(forTrips: [trip])
    router.previewTrip = trip
    router.setRoutingPresentationActive(true)
    carPlayService.router = router
    carPlayService.routingOwner = .carPlay
    carPlayService.mapState.didAttach(to: display)
    carPlayService.previewTemplate = template
    carPlayService.preparedToPreviewTrips = [trip]
    return (router, template, trip)
  }

  private func seedPhoneRoute() {
    MWMRouter.removePoints()
    MWMRouter.addPoint(MWMRoutePoint(cgPoint: CGPoint(x: 20, y: 20), title: "Phone start", subtitle: nil, type: .start, intermediateIndex: 0))
    MWMRouter.addPoint(MWMRoutePoint(cgPoint: CGPoint(x: 23, y: 23), title: "Phone finish", subtitle: nil, type: .finish, intermediateIndex: 0))
  }

  func testTripEstimatesPreserveRawSeconds() {
    let cases: [TimeInterval] = [0, 20.75, 1665, 94485, 2_147_483_647]
    for seconds in cases {
      let routeInfo = RouteInfo(timeToTarget: seconds,
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
      let estimates = routeInfo.tripTravelEstimates

      XCTAssertEqual(estimates.distanceRemaining, Measurement<UnitLength>(value: 25.2, unit: .kilometers))
      XCTAssertEqual(estimates.timeRemaining, seconds, "\(seconds) seconds")
      XCTAssertEqual(routeInfo.timeToTarget, seconds)
    }
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
    let bookmarkIds = manager.bookmarkIds(forCategory: category.categoryId).map(\.uint64Value)
    XCTAssertEqual(bookmarkIds.count, 3)
    for limit in [0, 2, 3, 4] {
      let snapshots = manager.bookmarks(forCategory: category.categoryId, limit: limit)
      let expectedIds = Array(bookmarkIds.prefix(limit))
      XCTAssertEqual(snapshots.map(\.bookmarkId), expectedIds, "limit=\(limit)")
    }
    manager.deleteCategory(category.categoryId)
    XCTAssertTrue(manager.bookmarks(forCategory: category.categoryId, limit: 2).isEmpty)
  }

  func testBookmarkImportRefreshesCategoryRows() throws {
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: { [template] }, popTo: { _ in
      XCTFail("Importing bookmarks must not request a return to a parent template.")
    })

    let category = try importBookmarkFixture()
    waitForBookmarkRefresh()

    let item = try categoryListItem(for: category, in: template)
    XCTAssertEqual(item.text, category.title)
    XCTAssertEqual(item.detailText, category.placesCountTitle())
  }

  func testBookmarkDeletionsRefreshRowsAndCategoryCountOncePerBatch() throws {
    let category = try importBookmarkFixture(bookmarkCount: 3)
    let parent = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    let previousCount = try categoryListItem(for: category, in: parent).detailText
    var refreshCount = 0
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: {
      refreshCount += 1
      return [parent, template]
    }, popTo: { _ in
      XCTFail("Deleting bookmarks from an existing category must not request a return to its parent.")
    })
    let manager = BookmarksManager.shared()
    let ids = manager.bookmarkIds(forCategory: category.categoryId)

    manager.deleteBookmark(ids[0].uint64Value)
    manager.deleteBookmark(ids[1].uint64Value)
    waitForBookmarkRefresh()

    let rows = template.sections.flatMap(\.items).compactMap { $0 as? CPListItem }
    let metadata = try XCTUnwrap((rows.first?.userInfo as? ListItemInfo)?.metadata as? BookmarkInfo)
    XCTAssertEqual(refreshCount, 1)
    XCTAssertEqual(rows.count, 1)
    XCTAssertEqual(metadata.bookmarkId, ids[2].uint64Value)
    let count = try categoryListItem(for: category, in: parent).detailText
    XCTAssertNotEqual(count, previousCount)
    XCTAssertEqual(count, category.placesCountTitle())

    manager.deleteBookmark(ids[2].uint64Value)
    waitForBookmarkRefresh()

    XCTAssertEqual(refreshCount, 2)
    XCTAssertTrue(template.sections.flatMap(\.items).isEmpty)
    XCTAssertFalse(parent.sections.flatMap(\.items).contains { $0.text == category.title })
  }

  func testDeletedBookmarkCategoryRequestsReturnToParentAndRefreshesIt() throws {
    let category = try importBookmarkFixture()
    let parent = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    let item = try categoryListItem(for: category, in: parent)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    var popTarget: CPTemplate?
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: { [parent, template] }, popTo: { popTarget = $0 })

    BookmarksManager.shared().deleteCategory(category.categoryId)
    waitForBookmarkRefresh()

    XCTAssertTrue(popTarget === parent)
    XCTAssertFalse(parent.sections.flatMap(\.items).contains { $0.text == item.text })
    XCTAssertTrue(template.sections.flatMap(\.items).isEmpty)
  }

  func testBookmarkReloadRefreshesExistingRowsCategoryNameAndCount() throws {
    let category = try importBookmarkFixture(bookmarkCount: 3)
    let renamed = "Renamed \(category.title)"
    let parent = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    let staleItem = try XCTUnwrap(template.sections.first?.items.first as? CPListItem)
    let staleInfo = try XCTUnwrap(staleItem.userInfo as? ListItemInfo)
    let staleMetadata = try XCTUnwrap(staleInfo.metadata as? BookmarkInfo)
    let previousCount = try categoryListItem(for: category, in: parent).detailText
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: { [parent, template] }, popTo: { _ in
      XCTFail("Reloading an existing category must not request a return to its parent.")
    })
    let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
      .appendingPathComponent("bookmarks", isDirectory: true)
    let url = try XCTUnwrap(FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: nil)
      .first { $0.deletingPathExtension().lastPathComponent == category.title })
    let kml = """
    <?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2"><Document><name>\(renamed)</name>
      <Placemark><name>Replacement 0</name><Point><coordinates>1,1</coordinates></Point></Placemark>
      <Placemark><name>Replacement 1</name><Point><coordinates>2,2</coordinates></Point></Placemark>
    </Document></kml>
    """
    try kml.write(to: url, atomically: true, encoding: .utf8)
    let manager = BookmarksManager.shared()
    waitForBookmarkLoad { manager.reloadCategory(atFilePath: url.path) }
    waitForBookmarkRefresh()

    XCTAssertTrue(manager.hasCategory(category.categoryId))
    XCTAssertEqual(category.title, renamed)
    XCTAssertEqual(template.sections.flatMap(\.items).compactMap(\.text).sorted(), ["Replacement 0", "Replacement 1"])
    XCTAssertEqual(manager.getCategoryMarksCount(category.categoryId), 2)
    let count = try categoryListItem(for: category, in: parent).detailText
    XCTAssertNotEqual(count, previousCount)
    XCTAssertEqual(count, category.placesCountTitle())
    XCTAssertFalse(manager.hasBookmark(staleMetadata.bookmarkId))
    var completionCount = 0
    carPlayService.handleListItemSelection(staleItem) { completionCount += 1 }
    XCTAssertEqual(completionCount, 1)
  }

  func testEndingBookmarkObservationCancelsQueuedRefresh() throws {
    let category = try importBookmarkFixture(bookmarkCount: 3)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    var refreshCount = 0
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: {
      refreshCount += 1
      return [template]
    }, popTo: { _ in
      XCTFail("An invalidated observer must not request a return to a parent template.")
    })
    let manager = BookmarksManager.shared()
    let ids = manager.bookmarkIds(forCategory: category.categoryId)

    manager.deleteBookmark(ids[0].uint64Value)
    bookmarkListObserver?.invalidate()
    manager.deleteBookmark(ids[1].uint64Value)
    waitForBookmarkRefresh()

    XCTAssertEqual(refreshCount, 0)
    XCTAssertEqual(template.sections.flatMap(\.items).count, 3)
  }

  func testBookmarkRefreshUsesTheCurrentTemplateStack() throws {
    let category = try importBookmarkFixture(bookmarkCount: 2)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    let replacement = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    var templates: [CPTemplate] = [template]
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: { templates }, popTo: { _ in
      XCTFail("Deleting a bookmark from an existing category must not request a return to its parent.")
    })
    let manager = BookmarksManager.shared()
    let ids = manager.bookmarkIds(forCategory: category.categoryId)

    manager.deleteBookmark(ids[0].uint64Value)
    templates = [replacement]
    waitForBookmarkRefresh()

    XCTAssertEqual(template.sections.flatMap(\.items).count, 2)
    XCTAssertEqual(replacement.sections.flatMap(\.items).count, 1)
  }

  private func waitForBookmarkRefresh() {
    let refreshed = expectation(description: "Queued bookmark refresh finished")
    DispatchQueue.main.async { refreshed.fulfill() }
    wait(for: [refreshed], timeout: 5)
  }

  private func waitForBookmarkLoad(_ startLoading: () -> Void) {
    let loaded = expectation(description: "Bookmark file loaded")
    let observer = BookmarksLoadObserver()
    observer.onFinished = { loaded.fulfill() }
    let manager = BookmarksManager.shared()
    manager.add(observer)
    defer { manager.remove(observer) }
    startLoading()
    wait(for: [loaded], timeout: 5)
  }

  func testCategoryRenameRefreshesTheOpenListInPlace() throws {
    let category = try importBookmarkFixture()
    let parent = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    var refreshCount = 0
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: {
      refreshCount += 1
      return [parent, template]
    }, popTo: { _ in
      XCTFail("Renaming a category must not request a return to a parent template.")
    })
    let renamed = "Renamed \(category.title)"

    BookmarksManager.shared().setCategory(category.categoryId, name: renamed)
    waitForBookmarkRefresh()

    XCTAssertEqual(refreshCount, 1)
    XCTAssertEqual(category.title, renamed)
  }

  func testBookmarkRenameRefreshesTheOpenList() throws {
    let category = try importBookmarkFixture()
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: category))
    var refreshCount = 0
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: {
      refreshCount += 1
      return [template]
    }, popTo: { _ in
      XCTFail("Renaming a bookmark must not request a return to a parent template.")
    })
    let manager = BookmarksManager.shared()
    let bookmark = manager.bookmark(withId: manager.bookmarkIds(forCategory: category.categoryId)[0].uint64Value)

    // Keep the color: a changed one becomes the default for new bookmarks.
    manager.updateBookmark(bookmark.bookmarkId, setGroupId: category.categoryId, title: "Renamed bookmark",
                           color: bookmark.bookmarkColor, description: "")
    waitForBookmarkRefresh()

    XCTAssertEqual(refreshCount, 1)
    XCTAssertEqual(manager.bookmark(withId: bookmark.bookmarkId).bookmarkName, "Renamed bookmark")
  }

  func testMovingBookmarkRefreshesBothCategoryCounts() throws {
    let source = try importBookmarkFixture(bookmarkCount: 2)
    let destination = try importBookmarkFixture()
    let parent = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: source))
    var refreshCount = 0
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: {
      refreshCount += 1
      return [parent, template]
    }, popTo: { _ in
      XCTFail("Moving a bookmark between existing categories must not request a return to a parent template.")
    })
    let manager = BookmarksManager.shared()
    let bookmarkId = manager.bookmarkIds(forCategory: source.categoryId)[0]

    manager.move(bookmarks: [bookmarkId], tracks: [], toGroupId: destination.categoryId)
    waitForBookmarkRefresh()

    XCTAssertEqual(refreshCount, 1)
    XCTAssertEqual(source.bookmarksCount, 1)
    XCTAssertEqual(destination.bookmarksCount, 2)
  }

  func testTrackDeletionRefreshesCategoryCounts() throws {
    let category = try importBookmarkFixture(trackCount: 1)
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    var refreshCount = 0
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: {
      refreshCount += 1
      return [template]
    }, popTo: { _ in
      XCTFail("Deleting a track from an existing category must not request a return to its parent.")
    })
    let manager = BookmarksManager.shared()
    let trackId = try XCTUnwrap(manager.trackIds(forCategory: category.categoryId).first)
    XCTAssertEqual(category.trackCount, 1)

    manager.deleteTrack(trackId.uint64Value)
    waitForBookmarkRefresh()

    XCTAssertEqual(refreshCount, 1)
    XCTAssertEqual(category.trackCount, 0)
  }

  func testBookmarkImportBatchRefreshesOnce() throws {
    let manager = BookmarksManager.shared()
    let prefix = "CarPlayBatch-\(UUID().uuidString)"
    let urls = try (0 ..< 2).map { index -> URL in
      let name = "\(prefix)-\(index)"
      let url = FileManager.default.temporaryDirectory.appendingPathComponent(name).appendingPathExtension("kml")
      let kml = """
      <?xml version="1.0"?><kml xmlns="http://www.opengis.net/kml/2.2"><Document>
      <name>\(name)</name><Placemark><Point><coordinates>0,0</coordinates></Point></Placemark>
      </Document></kml>
      """
      try kml.write(to: url, atomically: true, encoding: .utf8)
      return url
    }
    addTeardownBlock {
      for category in manager.sortedUserCategories() where category.title.hasPrefix(prefix) {
        manager.deleteCategory(category.categoryId)
      }
      manager.deleteRecentlyDeletedCategory(at: manager.getRecentlyDeletedCategories()
        .filter { $0.fileURL.lastPathComponent.hasPrefix(prefix) }.map(\.fileURL))
      for url in urls {
        try FileManager.default.removeItem(at: url)
      }
    }
    let template = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
    var refreshCount = 0
    bookmarkListObserver = CarPlayBookmarkListObserver(templates: {
      refreshCount += 1
      return [template]
    }, popTo: { _ in
      XCTFail("Importing bookmarks must not request a return to a parent template.")
    })
    let imported = expectation(description: "Both bookmark files imported")
    let observer = BookmarksLoadObserver()
    var queuedSecondFile = false
    // File success arrives while loading is active, so the next file joins the same batch.
    observer.onFileLoaded = {
      guard !queuedSecondFile else { return }
      queuedSecondFile = true
      manager.loadBookmarkFile(urls[1])
    }
    observer.onFinished = { imported.fulfill() }
    manager.add(observer)
    defer { manager.remove(observer) }

    manager.loadBookmarkFile(urls[0])
    wait(for: [imported], timeout: 5)
    waitForBookmarkRefresh()

    XCTAssertEqual(refreshCount, 1)
    XCTAssertEqual(manager.sortedUserCategories().filter { $0.title.hasPrefix(prefix) }.count, 2)
  }

  private func importBookmarkFixture(bookmarkCount: Int = 1, trackCount: Int = 0) throws -> BookmarkGroup {
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
    let tracks = (0 ..< trackCount).map { index in
      """
      <Placemark><name>CarPlay track \(index)</name>
        <LineString><coordinates>0,0 0.01,0.01</coordinates></LineString>
      </Placemark>
      """
    }.joined(separator: "\n")
    let kml = """
    <?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2">
      <Document>
        <name>\(name)</name>
        \(placemarks)
        \(tracks)
      </Document>
    </kml>
    """
    try kml.write(to: url, atomically: true, encoding: .utf8)
    addTeardownBlock { try FileManager.default.removeItem(at: url) }

    waitForBookmarkLoad { bookmarkManager.loadBookmarkFile(url) }

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
    return try categoryListItem(for: category, in: template)
  }

  private func categoryListItem(for category: BookmarkGroup, in template: CPListTemplate) throws -> CPListItem {
    try XCTUnwrap(template.sections.flatMap(\.items).compactMap { $0 as? CPListItem }.first { item in
      guard let info = item.userInfo as? ListItemInfo,
            let metadata = info.metadata as? CategoryInfo else { return false }
      return metadata.category.categoryId == category.categoryId
    })
  }

  private final class BookmarksLoadObserver: NSObject, BookmarksObserver {
    var onFinished: (() -> Void)?
    var onFileLoaded: (() -> Void)?

    func onBookmarksLoadFinished() { onFinished?() }
    func onBookmarksFileLoadSuccess() { onFileLoaded?() }
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
}
