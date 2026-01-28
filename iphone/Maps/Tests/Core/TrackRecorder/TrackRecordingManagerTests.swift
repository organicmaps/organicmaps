import XCTest
@testable import Organic_Maps__Debug_

final class TrackRecordingManagerTests: XCTestCase {

  private var trackRecordingManager: TrackRecordingManager!

  private var mockTrackRecorder: MockTrackRecorder.Type!
  private var mockLocationService: MockLocationService.Type!
  private var mockActivityManager: MockTrackRecordingActivityManager!

  override func setUp() {
    super.setUp()
    mockTrackRecorder = MockTrackRecorder.self
    mockLocationService = MockLocationService.self
    mockActivityManager = MockTrackRecordingActivityManager()

    trackRecordingManager = TrackRecordingManager(
      trackRecorder: mockTrackRecorder,
      locationService: mockLocationService,
      activityManager: mockActivityManager
    )
  }

  override func tearDown() {
    trackRecordingManager = nil
    mockTrackRecorder.reset()
    mockLocationService.reset()
    mockActivityManager = nil
    super.tearDown()
  }

  func test_GivenInitialSetup_WhenLocationEnabled_ThenStateIsInactive() {
    mockLocationService.locationIsProhibited = false
    mockTrackRecorder.trackRecordingIsEnabled = false

    trackRecordingManager.setup()
    XCTAssertTrue(trackRecordingManager.recordingState == .inactive)
  }

  func test_GivenInitialSetup_WhenLocationDisabled_ThenShouldHandleErrorAndIncativeState() {
    mockLocationService.locationIsProhibited = true

    trackRecordingManager.setup()

    XCTAssertTrue(mockLocationService.checkLocationStatusCalled)
    XCTAssertTrue(trackRecordingManager.recordingState == .inactive)
  }

  func test_GivenStartRecording_WhenLocationEnabled_ThenSuccess() {
    mockLocationService.locationIsProhibited = false
    mockTrackRecorder.trackRecordingIsEnabled = false

    trackRecordingManager.start()

    XCTAssertTrue(mockTrackRecorder.startTrackRecordingCalled)
    XCTAssertTrue(mockActivityManager.startCalled)
    XCTAssertTrue(trackRecordingManager.recordingState == .active)
  }

  func test_GivenStartRecording_WhenLocationDisabled_ThenShouldFail() {
    mockLocationService.locationIsProhibited = true
    let expectation = expectation(description: "Location is prohibited")
    trackRecordingManager.start() { result in
      switch result {
      case .success:
        XCTFail("Should not succeed")
      case .failure(let error):
        switch error {
        case LocationError.locationIsProhibited:
          expectation.fulfill()
        default:
          XCTFail("Unexpected error: \(error)")
        }
      }
    }
    wait(for: [expectation], timeout: 1.0)
    XCTAssertFalse(self.mockTrackRecorder.startTrackRecordingCalled)
    XCTAssertTrue(trackRecordingManager.recordingState == .inactive)
  }

  func test_GivenStopRecording_WhenLocationEnabled_ThenSuccess() {
    mockTrackRecorder.trackRecordingIsEnabled = true
    mockTrackRecorder.trackRecordingIsEmpty = false

    trackRecordingManager.stopAndSave(withName: "Test Track") { result in
      switch result {
      case .success:
        XCTAssertTrue(true)
      case .trackIsEmpty:
        XCTFail("Track should not be empty")
      }
    }
    XCTAssertTrue(mockTrackRecorder.stopTrackRecordingCalled)
    XCTAssertTrue(mockTrackRecorder.saveTrackRecordingCalled)
    XCTAssertTrue(mockActivityManager.stopCalled)
    XCTAssertTrue(trackRecordingManager.recordingState == .inactive)
  }

  func test_GivenStopRecording_WhenTrackIsEmpty_ThenShouldFail() {
    mockTrackRecorder.trackRecordingIsEnabled = true
    mockTrackRecorder.trackRecordingIsEmpty = true
    let expectation = expectation(description: "Track recording should be empty")
    trackRecordingManager.stopAndSave(withName: "Test Track") { result in
      switch result {
      case .success:
        XCTFail("Should not succeed")
      case .trackIsEmpty:
        expectation.fulfill()
      }
    }
    wait(for: [expectation], timeout: 1.0)
    XCTAssertFalse(mockTrackRecorder.saveTrackRecordingCalled)
    XCTAssertTrue(trackRecordingManager.recordingState == .inactive)
  }
}

// MARK: - Mock Classes

private final class MockTrackRecorder: TrackRecorder {
  static var trackRecordingIsEnabled = false
  static var trackRecordingIsEmpty = false
  static var startTrackRecordingCalled = false
  static var stopTrackRecordingCalled = false
  static var saveTrackRecordingCalled = false

  static func reset() {
    trackRecordingIsEnabled = false
    trackRecordingIsEmpty = false
    startTrackRecordingCalled = false
    stopTrackRecordingCalled = false
    saveTrackRecordingCalled = false
  }

  static func isTrackRecordingEnabled() -> Bool {
    return trackRecordingIsEnabled
  }

  static func isTrackRecordingEmpty() -> Bool {
    return trackRecordingIsEmpty
  }

  static func startTrackRecording() {
    startTrackRecordingCalled = true
    trackRecordingIsEnabled = true
  }

  static func stopTrackRecording() {
    stopTrackRecordingCalled = true
    trackRecordingIsEnabled = false
  }

  static func saveTrackRecording(withName name: String) {
    saveTrackRecordingCalled = true
  }

  static func setTrackRecordingUpdateHandler(_ handler: ((TrackInfo) -> Void)?) {}

  static func trackRecordingElevationInfo() -> ElevationProfileData {
    ElevationProfileData()
  }
}

private final class MockLocationService: LocationService {
  static var locationIsProhibited = false
  static var checkLocationStatusCalled = false

  static func reset() {
    locationIsProhibited = false
    checkLocationStatusCalled = false
  }

  static func isLocationProhibited() -> Bool {
    return locationIsProhibited
  }

  static func checkLocationStatus() {
    checkLocationStatusCalled = true
  }
}

final class MockTrackRecordingActivityManager: TrackRecordingActivityManager {
  var startCalled = false
  var stopCalled = false

  func start(with info: TrackInfo) throws {
    startCalled = true
  }

  func stop() {
    stopCalled = true
  }

  func update(_ info: TrackInfo) {}
}
