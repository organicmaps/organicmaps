@testable import Organic_Maps__Debug_
import XCTest

final class TrackPlacePageTests: XCTestCase {
  func testElevationUpdatesSurviveReturningFromUnchangedEditor() throws {
    let bookmarksManager = BookmarksManager.shared()
    let trackId = try importTrack()
    bookmarksManager.setElevationActivePointDistance(0, trackId: trackId)
    let data = TrackPlacePageData(trackId: trackId)
    let page = PlacePageBuilder.build(for: data)
    let navigationController = UINavigationController(rootViewController: page)
    let scene = try XCTUnwrap(MapsAppDelegate.theApp().window?.windowScene)
    let window = UIWindow(windowScene: scene)
    window.rootViewController = navigationController
    window.makeKeyAndVisible()
    addTeardownBlock {
      window.isHidden = true
      MapsAppDelegate.theApp().window?.makeKey()
    }
    waitForAnimations()
    let interactor = try XCTUnwrap(page.interactor as? PlacePageInteractor)
    let presenter = ActivePointPresenterSpy()
    interactor.trackActivePointPresenter = presenter

    bookmarksManager.setElevationActivePointDistance(10, trackId: trackId)
    XCTAssertEqual(presenter.activePointDistances, [10])

    var edited: Bool?
    let editor = EditTrackViewController(trackId: trackId) { edited = $0 }
    navigationController.pushViewController(editor, animated: false)
    waitForAnimations()
    XCTAssertTrue(navigationController.topViewController === editor)
    navigationController.popViewController(animated: false)
    waitForAnimations()

    XCTAssertEqual(edited, false)
    XCTAssertTrue(navigationController.topViewController === page)
    XCTAssertNotNil(page.view.window)
    XCTAssertTrue(page.interactor === interactor, "Returning without edits must keep the existing interactor")
    bookmarksManager.setElevationActivePointDistance(20, trackId: trackId)
    XCTAssertEqual(presenter.activePointDistances, [10, 20])

    let distancesBeforeClose = presenter.activePointDistances
    page.close {
      navigationController.setViewControllers([UIViewController()], animated: false)
    }
    waitForAnimations()
    XCTAssertNil(page.interactor)
    bookmarksManager.setElevationActivePointDistance(30, trackId: trackId)
    XCTAssertEqual(presenter.activePointDistances, distancesBeforeClose, "Closing the page must unsubscribe even while it is retained")
  }

  private func importTrack() throws -> MWMTrackID {
    let bookmarksManager = BookmarksManager.shared()
    let name = "TrackPlacePageTests-\(UUID().uuidString)"
    let url = FileManager.default.temporaryDirectory.appendingPathComponent(name).appendingPathExtension("kml")
    let kml = """
    <?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2">
      <Document>
        <name>\(name)</name>
        <Placemark>
          <name>Elevation test track</name>
          <LineString><coordinates>13.405,52.52,10 13.406,52.521,20 13.407,52.522,15</coordinates></LineString>
        </Placemark>
      </Document>
    </kml>
    """
    try kml.write(to: url, atomically: true, encoding: .utf8)
    addTeardownBlock { try FileManager.default.removeItem(at: url) }

    let imported = expectation(description: "Track imported")
    let observer = TrackLoadObserver()
    observer.onFinished = { imported.fulfill() }
    bookmarksManager.add(observer)
    defer { bookmarksManager.remove(observer) }
    bookmarksManager.loadBookmarkFile(url)
    wait(for: [imported], timeout: 5)

    let category = try XCTUnwrap(bookmarksManager.sortedUserCategories().first { $0.title == name })
    let categoryId = category.categoryId
    addTeardownBlock {
      bookmarksManager.deleteCategory(categoryId)
      let trashedURLs = bookmarksManager.getRecentlyDeletedCategories()
        .filter { $0.fileURL.lastPathComponent.hasPrefix(name) }
        .map(\.fileURL)
      bookmarksManager.deleteRecentlyDeletedCategory(at: trashedURLs)
    }
    return try XCTUnwrap(bookmarksManager.tracks(forGroup: categoryId).first).trackId
  }

  private func waitForAnimations() {
    let settled = expectation(description: "Navigation and Place Page animations finished")
    DispatchQueue.main.asyncAfter(deadline: .now() + AppConstants.defaultAnimationDuration * 2) {
      settled.fulfill()
    }
    wait(for: [settled], timeout: 2)
  }

  /// Track selection requires Drape, which is disabled in XCTest. Supply only the saved track identity.
  private final class TrackPlacePageData: PlacePageData {
    private let savedTrackData: PlacePageTrackData

    init(trackId: MWMTrackID) {
      savedTrackData = TrackData(trackId: trackId)
      super.init(trackInfo: TrackInfo.empty(), elevationInfo: nil)
    }

    override var objectType: PlacePageObjectType { .track }
    override var trackData: PlacePageTrackData? { savedTrackData }
  }

  private final class TrackData: PlacePageTrackData {
    private let savedTrackId: MWMTrackID

    init(trackId: MWMTrackID) {
      savedTrackId = trackId
      super.init(trackInfo: TrackInfo.empty(), elevationInfo: nil, onActivePointChanged: {})
    }

    override var trackId: MWMTrackID { savedTrackId }
  }

  private final class TrackLoadObserver: NSObject, BookmarksObserver {
    var onFinished: (() -> Void)?

    func onBookmarksLoadFinished() { onFinished?() }
  }

  private final class ActivePointPresenterSpy: TrackActivePointPresenter {
    var activePointDistances: [Double] = []

    func updateActivePointDistance(_ distance: Double) { activePointDistances.append(distance) }
    func updateMyPositionDistance(_: Double) {}
  }
}
