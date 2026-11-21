@testable import Organic_Maps__Debug_
import XCTest

final class PlacePageInteractorTests: XCTestCase {
  @MainActor
  func test_DeleteInvalidRecordingDiscardsImmediately() {
    FrameworkHelper.discardTrackRecording()
    defer { FrameworkHelper.discardTrackRecording() }
    FrameworkHelper.startTrackRecording()
    XCTAssertTrue(FrameworkHelper.isTrackRecordingEnabled())

    let data = PlacePageData(trackInfo: .empty(), elevationInfo: nil)
    let interactor = PlacePageInteractor(data: data)
    interactor.actionBar(ActionBarViewController(), didPressButton: .deleteTrackRecording)

    XCTAssertFalse(FrameworkHelper.isTrackRecordingEnabled())
  }
}
