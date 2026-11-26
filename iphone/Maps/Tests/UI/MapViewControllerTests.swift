@testable import Chart
@testable import Organic_Maps__Debug_
import XCTest

final class MapViewControllerTests: XCTestCase {
  @MainActor
  func test_SynchronousSavedTrackSelectionClosesRecordingButton() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let trackId = try importTrack()

    MockTrackRecorder.reset()
    let recordingManager = TrackRecordingManager(trackRecorder: MockTrackRecorder.self,
                                                 locationService: MockLocationService.self,
                                                 activityManager: nil)
    let originalManager = map.value(forKey: "trackRecordingManager")
    map.setValue(recordingManager, forKey: "trackRecordingManager")
    defer {
      map.dismissPlacePage()
      map.controlsManager.setTrackRecordingButtonState(.closed)
      map.setValue(originalManager, forKey: "trackRecordingManager")
      MockTrackRecorder.reset()
    }

    MockTrackRecorder.trackRecordingIsEnabled = true
    MockTrackRecorder.saveResult = true
    map.controlsManager.setTrackRecordingButtonState(.visible)
    map.showTrackRecordingPlacePage()
    XCTAssertTrue(recordingManager.contains(map))
    XCTAssertNotNil(map.controlsManager.trackRecordingButton)

    MockTrackRecorder.onSave = { FrameworkHelper.showTrack(trackId) }
    recordingManager.stopAndSave()

    XCTAssertEqual(PlacePageData(localizationProvider: OpeinigHoursLocalization()).trackData?.trackId, trackId)
    XCTAssertFalse(recordingManager.contains(map))
    XCTAssertNil(map.controlsManager.trackRecordingButton)
  }

  func testTrackReverseAfterClosingSearch() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let track = try importTrack(lines: ["19,0 19.001,0", "20,0 20.001,0"])
    MWMRouter.stopRouting()
    defer { MWMRouter.stopRouting() }
    MWMRouter.subscribeToEvents()
    BookmarksManager.shared().setElevationActivePointDistance(0, trackId: track)
    XCTAssertEqual(MWMRouter.navigateTrack(track), .ok)

    waitForPreview()
    let dashboard = try XCTUnwrap(map.children.compactMap { $0 as? NavigationDashboardViewController }.last)
    let reverse = try reverseButton(in: dashboard)
    XCTAssertFalse(reverse.isEnabled)
    XCTAssertEqual(reverse.accessibilityValue, L("navigate_track_section_finish"))

    XCTAssertEqual(MWMRouter.updateFollowedTrackEntry(atDistance: 40), .ok)
    waitForPreview()
    map.searchManager.startSearching(isRouting: false)
    map.searchManager.close()
    XCTAssertTrue(reverse.isEnabled)
    reverse.sendActions(for: .touchUpInside)
    waitForPreview()
    XCTAssertEqual(MWMRouter.followedTrackInfo()?.isReversed, true)
    XCTAssertEqual(reverse.accessibilityValue, L("navigate_track_start"))
  }

  func testLoopReverseMovesTheChartMarkerToTheOtherTrailheadOccurrence() throws {
    let map = try XCTUnwrap(MapViewController.shared())
    map.loadViewIfNeeded()
    let track = try importTrack(lines: ["19,0,100 19.001,0,125 19.001,0.001,150 19,0,100"])
    MWMRouter.stopRouting()
    defer { MWMRouter.stopRouting() }
    MWMRouter.subscribeToEvents()
    BookmarksManager.shared().setElevationActivePointDistance(0, trackId: track)
    XCTAssertEqual(MWMRouter.navigateTrack(track), .ok)
    waitForPreview()

    let dashboard = try XCTUnwrap(map.children.compactMap { $0 as? NavigationDashboardViewController }.last)
    func markerFraction() throws -> CGFloat {
      let chart = try XCTUnwrap(dashboard.view.firstDescendant { $0 is ChartView } as? ChartView)
      return chart.chartInfoView.infoX
    }
    XCTAssertEqual(try markerFraction(), 0, accuracy: 0.001)
    let reverse = try reverseButton(in: dashboard)
    XCTAssertTrue(reverse.isEnabled)
    reverse.sendActions(for: .touchUpInside)
    waitForPreview()
    XCTAssertEqual(MWMRouter.followedTrackInfo()?.isReversed, true)
    XCTAssertEqual(try markerFraction(), 1, accuracy: 0.001)
  }

  private func reverseButton(in dashboard: NavigationDashboardViewController) throws -> UIButton {
    try XCTUnwrap(dashboard.view.firstDescendant { $0 is UIButton && $0.accessibilityLabel == L("reverse_route") } as? UIButton)
  }

  private func waitForPreview() {
    wait(for: [XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in MWMRouter.isRouteBuilt() }, object: nil)], timeout: 5)
  }

  private func importTrack(lines: [String] = ["19,0 19.001,0"]) throws -> UInt64 {
    let bookmarks = BookmarksManager.shared()
    let name = "RecordingTest-\(UUID().uuidString)"
    let url = FileManager.default.temporaryDirectory.appendingPathComponent(name).appendingPathExtension("kml")
    let geometry = lines.map { "<LineString><coordinates>\($0)</coordinates></LineString>" }.joined()
    let kml = """
    <kml xmlns="http://www.opengis.net/kml/2.2"><Document><name>\(name)</name>
      <Placemark><name>Saved track</name><MultiGeometry>\(geometry)</MultiGeometry></Placemark>
    </Document></kml>
    """
    try kml.write(to: url, atomically: true, encoding: .utf8)
    defer { try? FileManager.default.removeItem(at: url) }

    let loaded = expectation(description: "Track loaded")
    let observer = TrackImportObserver(loaded)
    bookmarks.add(observer)
    defer { bookmarks.remove(observer) }
    bookmarks.loadBookmarkFile(url)
    wait(for: [loaded], timeout: 5)

    let group = try XCTUnwrap(bookmarks.sortedUserCategories().first { $0.title == name })
    let groupId = group.categoryId
    addTeardownBlock {
      bookmarks.deleteCategory(groupId)
      let trashed = bookmarks.getRecentlyDeletedCategories().filter { $0.fileURL.lastPathComponent.hasPrefix(name) }
      bookmarks.deleteRecentlyDeletedCategory(at: trashed.map(\.fileURL))
    }
    return try XCTUnwrap(bookmarks.trackIds(forCategory: groupId).first).uint64Value
  }
}

private final class TrackImportObserver: NSObject, BookmarksObserver {
  private let loaded: XCTestExpectation

  init(_ loaded: XCTestExpectation) { self.loaded = loaded }

  func onBookmarksLoadFinished() { loaded.fulfill() }
}
