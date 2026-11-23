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
    MockTrackRecorder.saveResult = .success
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

  private func importTrack() throws -> UInt64 {
    let bookmarks = BookmarksManager.shared()
    let name = "RecordingTest-\(UUID().uuidString)"
    let url = FileManager.default.temporaryDirectory.appendingPathComponent(name).appendingPathExtension("kml")
    let kml = """
    <kml xmlns="http://www.opengis.net/kml/2.2"><Document><name>\(name)</name>
      <Placemark><name>Saved track</name><LineString><coordinates>19,0 19.001,0</coordinates></LineString></Placemark>
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
