@testable import Organic_Maps__Debug_
import XCTest

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
    trackRecordingManager.start { result in
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
    XCTAssertFalse(mockTrackRecorder.startTrackRecordingCalled)
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
    trackRecordingIsEnabled
  }

  static func isTrackRecordingEmpty() -> Bool {
    trackRecordingIsEmpty
  }

  static func startTrackRecording() {
    startTrackRecordingCalled = true
    trackRecordingIsEnabled = true
  }

  static func stopTrackRecording() {
    stopTrackRecordingCalled = true
    trackRecordingIsEnabled = false
  }

  static func saveTrackRecording(withName _: String) {
    saveTrackRecordingCalled = true
  }

  static func setTrackRecordingUpdateHandler(_: ((TrackInfo) -> Void)?) {}
}

private final class MockLocationService: LocationService {
  static var locationIsProhibited = false
  static var checkLocationStatusCalled = false

  static func reset() {
    locationIsProhibited = false
    checkLocationStatusCalled = false
  }

  static func isLocationProhibited() -> Bool {
    locationIsProhibited
  }

  static func checkLocationStatus() {
    checkLocationStatusCalled = true
  }
}

final class MockTrackRecordingActivityManager: TrackRecordingActivityManager {
  var startCalled = false
  var stopCalled = false

  func start(with _: TrackInfo) throws {
    startCalled = true
  }

  func stop() {
    stopCalled = true
  }

  func update(_: TrackInfo) {}
}

/// Profile requests are independent of the scalar recording/Live Activity stream.
final class ElevationProfileVisibilityTests: XCTestCase {
  private final class Counter {
    private let lock = NSLock()
    private var count = 0

    func increment() -> Int {
      lock.lock()
      defer { lock.unlock() }
      count += 1
      return count
    }

    var value: Int {
      lock.lock()
      defer { lock.unlock() }
      return count
    }
  }

  private func makeChart() -> (UIWindow, UIScrollView, ElevationProfileViewController) {
    let window = UIWindow(frame: CGRect(x: 0, y: 0, width: 320, height: 640))
    let root = UIViewController()
    window.rootViewController = root
    let viewport = UIScrollView(frame: CGRect(x: 0, y: 0, width: 320, height: 260))
    viewport.contentSize = CGSize(width: 320, height: 1000)
    root.view.addSubview(viewport)
    let chart = ElevationProfileViewController(presentationStyle: .track)
    chart.loadViewIfNeeded()
    chart.view.frame = CGRect(x: 0, y: 400, width: 320, height: 236)
    viewport.addSubview(chart.view)
    chart.view.layoutIfNeeded()
    window.isHidden = false
    return (window, viewport, chart)
  }

  func testHiddenPlotDoesNotLoadAndShowingNeedsNoNewGpsFix() {
    let (window, viewport, chart) = makeChart()
    defer { window.isHidden = true }
    let calls = Counter()
    let loaded = expectation(description: "Visible graph loaded")
    let value = ElevationProfileData()
    chart.profileLoader = { _, _ in
      _ = calls.increment()
      return value
    }
    chart.onProfileDataChanged = { data in
      XCTAssertTrue(data === value)
      loaded.fulfill()
    }
    chart.updateProfileVisibility(in: viewport, active: true)
    for _ in 0 ..< 100 {
      chart.setNeedsProfileUpdate()
    }
    XCTAssertEqual(calls.value, 0)
    viewport.contentOffset.y = 400
    chart.updateProfileVisibility(in: viewport, active: true)
    wait(for: [loaded], timeout: 5)
    XCTAssertEqual(calls.value, 1)
    chart.updateProfileVisibility(in: viewport, active: false)
    chart.updateProfileVisibility(in: viewport, active: true)
    XCTAssertEqual(calls.value, 1)
  }

  func testHidingCancelsAndDiscardsAnOldCompletionAfterReopening() {
    let (window, viewport, chart) = makeChart()
    defer { window.isHidden = true }
    viewport.contentOffset.y = 400
    let calls = Counter()
    let started = expectation(description: "First request started")
    let finished = expectation(description: "Cancelled request finished")
    let applied = expectation(description: "Reopened graph loaded")
    let release = DispatchSemaphore(value: 0)
    defer { release.signal() }
    let stale = ElevationProfileData()
    let latest = ElevationProfileData()
    chart.profileLoader = { _, isCancelled in
      if calls.increment() == 1 {
        started.fulfill()
        _ = release.wait(timeout: .now() + 5)
        XCTAssertTrue(isCancelled())
        finished.fulfill()
        return stale
      }
      return latest
    }
    chart.onProfileDataChanged = { data in
      XCTAssertTrue(data === latest)
      applied.fulfill()
    }
    chart.updateProfileVisibility(in: viewport, active: true)
    wait(for: [started], timeout: 5)
    for _ in 0 ..< 100 {
      chart.setNeedsProfileUpdate()
    }
    XCTAssertEqual(calls.value, 1)
    chart.updateProfileVisibility(in: viewport, active: false)
    release.signal()
    wait(for: [finished], timeout: 5)
    chart.updateProfileVisibility(in: viewport, active: true)
    wait(for: [applied], timeout: 5)
    XCTAssertEqual(calls.value, 2)
    chart.updateProfileVisibility(in: viewport, active: false)
  }
}
