@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class SynchronizationManagerTests: XCTestCase {
  private var manager: iCloudSynchronizaionManager!
  private var cloudMonitor: CloudMonitorStub!
  private var localMonitor: LocalMonitorStub!
  private var resolver: ResolverStub!
  private var fingerprintProvider: FingerprintProviderMock!
  private var clock: ActiveSynchronizationClock!

  override func setUp() {
    super.setUp()
    cloudMonitor = CloudMonitorStub()
    localMonitor = LocalMonitorStub()
    resolver = ResolverStub()
    fingerprintProvider = FingerprintProviderMock()
    clock = ActiveSynchronizationClock()
    manager = iCloudSynchronizaionManager(fileManager: .default,
                                          settings: EnabledSynchronizationSettings.self,
                                          bookmarksManager: .shared(),
                                          cloudDirectoryMonitor: cloudMonitor,
                                          localDirectoryMonitor: localMonitor,
                                          stateResolver: resolver,
                                          stateStore: SynchronizedStateStoreMock(),
                                          fingerprintProvider: fingerprintProvider,
                                          clock: clock)
    manager.start()
  }

  override func tearDown() {
    NotificationCenter.default.removeObserver(manager!)
    clock.pause()
    manager = nil
    cloudMonitor = nil
    localMonitor = nil
    resolver = nil
    fingerprintProvider = nil
    clock = nil
    super.tearDown()
  }

  func testStartCompletedInBackgroundResumesItsClockOnForeground() {
    post(UIApplication.didBecomeActiveNotification)
    XCTAssertEqual(cloudMonitor.state, .starting)
    post(UIApplication.didEnterBackgroundNotification)
    XCTAssertFalse(clock.isRunning)

    cloudMonitor.finishStart()
    XCTAssertEqual(localMonitor.state, .paused)
    XCTAssertEqual(cloudMonitor.state, .paused)

    post(UIApplication.didBecomeActiveNotification)

    XCTAssertTrue(clock.isRunning)
    XCTAssertEqual(localMonitor.state, .started)
    XCTAssertEqual(cloudMonitor.state, .started)
    XCTAssertEqual(localMonitor.startCount, 1)
    XCTAssertEqual(cloudMonitor.startCount, 1)
    XCTAssertEqual(localMonitor.refreshCount, 1)
  }

  func testForegroundWhileStartingDoesNotStartTheLocalMonitorEarly() {
    post(UIApplication.didBecomeActiveNotification)
    post(UIApplication.didEnterBackgroundNotification)
    post(UIApplication.didBecomeActiveNotification)

    XCTAssertTrue(clock.isRunning)
    XCTAssertEqual(localMonitor.state, .stopped)
    XCTAssertEqual(cloudMonitor.startCount, 1)

    cloudMonitor.finishStart()

    XCTAssertEqual(localMonitor.startCount, 1)
    XCTAssertEqual(localMonitor.state, .started)
  }

  func testFingerprintCallbackDoesNotReconcilePausedSnapshots() {
    post(UIApplication.didBecomeActiveNotification)
    cloudMonitor.finishStart()
    post(UIApplication.didEnterBackgroundNotification)
    let eventsBeforeCallback = resolver.events.count

    fingerprintProvider.onContentsMayBeKnown?()

    XCTAssertEqual(resolver.events.count, eventsBeforeCallback)
  }

  func testAccountChangeResetsTheFingerprintProvider() {
    post(UIApplication.didBecomeActiveNotification)
    cloudMonitor.finishStart()

    post(.NSUbiquityIdentityDidChange)

    XCTAssertEqual(fingerprintProvider.resetCount, 1)
    XCTAssertEqual(cloudMonitor.startCount, 2)
  }

  func testCleanSnapshotClearsAnUnmappedSystemError() {
    let error = NSError(domain: NSCocoaErrorDomain, code: NSFileWriteOutOfSpaceError)
    var states = [SynchronizationManagerState]()
    manager.addObserver(self) { states.append($0) }
    manager.updateSynchronizationError(error)
    manager.didReceiveCloudSnapshot(CloudSnapshot(items: []))
    let delivered = expectation(description: "The queued observer callbacks have run")
    DispatchQueue.main.async { delivered.fulfill() }
    wait(for: [delivered], timeout: 1)

    XCTAssertEqual(states.count, 3)
    XCTAssertEqual(states.dropFirst().first?.error, error)
    XCTAssertNil(states.last?.error)
  }

  func testConfirmedDeletionTrashesAFileThatHasNoLoadedCategory() throws {
    let bookmarksManager = BookmarksManager.shared()
    XCTAssertTrue(bookmarksManager.areBookmarksLoaded())
    let name = "UnparsedSyncTests-\(UUID().uuidString).kml"
    let fileUrl = FileManager.default.bookmarksDirectoryUrl.appendingPathComponent(name)
    let trashUrl = fileUrl.deletingLastPathComponent().deletingLastPathComponent()
      .appendingPathComponent(".Trash").appendingPathComponent(name)
    let data = Data("unsupported KML content".utf8)
    try data.write(to: fileUrl)
    addTeardownBlock {
      try? FileManager.default.removeItem(at: fileUrl)
      try? FileManager.default.removeItem(at: trashUrl)
    }

    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path))

    XCTAssertFalse(FileManager.default.fileExists(atPath: fileUrl.path))
    XCTAssertEqual(try Data(contentsOf: trashUrl), data)
    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path), "An absent file is already deleted")
  }

  private func post(_ name: Notification.Name) {
    NotificationCenter.default.post(name: name, object: nil)
  }
}

private final class EnabledSynchronizationSettings: Settings {
  override class func iCLoudSynchronizationEnabled() -> Bool { true }
}

private final class CloudMonitorStub: CloudDirectoryMonitor {
  weak var delegate: CloudDirectoryMonitorDelegate?
  var state: DirectoryMonitorState = .stopped
  var cloudIdentity: Data? { nil }
  private var startCompletion: ((Result<URL, Error>) -> Void)?
  private(set) var startCount = 0

  func start(completion: ((Result<URL, Error>) -> Void)?) {
    state = .starting
    startCount += 1
    startCompletion = completion
  }

  func finishStart() {
    state = .started
    startCompletion?(.success(URL(fileURLWithPath: "/cloud")))
    startCompletion = nil
  }

  func stop() { state = .stopped }

  func pause() {
    if state == .started {
      state = .paused
    }
  }

  func resume() {
    if state == .paused {
      state = .started
    }
  }

  func refresh() {}
  func isCloudAvailable() -> Bool { true }
}

private final class LocalMonitorStub: LocalDirectoryMonitor {
  weak var delegate: LocalDirectoryMonitorDelegate?
  var state: DirectoryMonitorState = .stopped
  let directory = URL(fileURLWithPath: "/local")
  private(set) var startCount = 0
  private(set) var refreshCount = 0

  func start(completion: ((Result<URL, Error>) -> Void)?) {
    state = .started
    startCount += 1
    completion?(.success(directory))
  }

  func stop() { state = .stopped }

  func pause() {
    if state == .started {
      state = .paused
    }
  }

  func resume() {
    if state == .paused {
      state = .started
    }
  }

  func refresh() {
    if state == .started {
      refreshCount += 1
    }
  }
}

private final class ResolverStub: SynchronizationStateResolver {
  let hasPendingConfirmations = false
  private(set) var events = [IncomingSynchronizationEvent]()

  func resolveEvent(_ event: IncomingSynchronizationEvent) -> [OutgoingSynchronizationEvent] {
    events.append(event)
    return []
  }

  func authorizes(_: OutgoingSynchronizationEvent) -> Bool { true }
  func resetState() {}
}
