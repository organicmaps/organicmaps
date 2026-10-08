@testable import Organic_Maps__Debug_
import XCTest

typealias UbiquityIdentityToken = NSCoding & NSCopying & NSObjectProtocol

class iCloudDirectoryMonitorTests: XCTestCase {
  var cloudMonitor: iCloudDocumentsMonitor!
  var mockFileManager: FileManagerMock!
  var mockDelegate: UbiquitousDirectoryMonitorDelegateMock!
  var cloudContainerIdentifier: String = "iCloud.app.organicmaps.debug"

  override func setUp() {
    super.setUp()
    mockFileManager = FileManagerMock()
    mockDelegate = UbiquitousDirectoryMonitorDelegateMock()
    cloudMonitor = iCloudDocumentsMonitor(fileManager: mockFileManager, cloudContainerIdentifier: cloudContainerIdentifier, fileType: .kml)
    cloudMonitor.delegate = mockDelegate
  }

  override func tearDown() {
    cloudMonitor = nil
    mockFileManager = nil
    mockDelegate = nil
    super.tearDown()
  }

  func testInitialization() {
    XCTAssertNotNil(cloudMonitor)
    XCTAssertEqual(cloudMonitor.containerIdentifier, cloudContainerIdentifier)
  }

  func testCloudAvailability() {
    mockFileManager.stubUbiquityIdentityToken = NSString(string: "mockToken")
    XCTAssertTrue(cloudMonitor.isCloudAvailable())

    mockFileManager.stubUbiquityIdentityToken = nil
    XCTAssertFalse(cloudMonitor.isCloudAvailable())
  }

  func testStartThatIsNotStoppedMeanwhileCompletes() {
    let fileManager = FileManagerMock()
    let monitor = makeMonitor(with: fileManager)

    let started = expectation(description: "The start completes")
    monitor.start { result in
      guard case .success = result else { return XCTFail("The start must succeed, got \(result)") }
      started.fulfill()
    }
    wait(for: [started], timeout: 5)

    XCTAssertEqual(monitor.state, .started)
  }

  /// Asking iCloud where the directory is takes a while, and the synchronization can be switched off, fail or
  /// change the account meanwhile: the answer belongs to a start that nothing waits for anymore.
  func testStartThatIsStoppedMeanwhileStartsNothing() {
    let fileManager = FileManagerMock()
    fileManager.containerUrlLookup.enter()
    let monitor = makeMonitor(with: fileManager)

    let started = expectation(description: "The start completes")
    started.isInverted = true
    monitor.start { _ in started.fulfill() }
    XCTAssertEqual(monitor.state, .starting)

    monitor.stop()
    XCTAssertEqual(monitor.state, .stopped)
    fileManager.containerUrlLookup.leave()

    wait(for: [started], timeout: 1)
    XCTAssertEqual(monitor.state, .stopped)
  }

  func testCancelledLookupCannotCacheThePreviousAccountsDirectory() {
    let fileManager = FileManagerMock()
    let oldContainer = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    let newContainer = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    let lookupStarted = expectation(description: "The old account lookup starts")
    let releaseLookup = DispatchGroup()
    releaseLookup.enter()
    fileManager.containerUrlLookupHandler = {
      lookupStarted.fulfill()
      releaseLookup.wait()
      return oldContainer
    }
    let monitor = makeMonitor(with: fileManager)
    addTeardownBlock {
      try? FileManager.default.removeItem(at: oldContainer)
      try? FileManager.default.removeItem(at: newContainer)
    }
    let cancelled = expectation(description: "The cancelled start does not complete")
    cancelled.isInverted = true
    monitor.start { _ in cancelled.fulfill() }
    wait(for: [lookupStarted], timeout: 5)
    monitor.stop()
    releaseLookup.leave()
    wait(for: [cancelled], timeout: 1)

    fileManager.containerUrlLookupHandler = { newContainer }
    let started = expectation(description: "The new account starts")
    monitor.start { result in
      guard case .success(let url) = result else { return XCTFail("Unexpected result: \(result)") }
      XCTAssertEqual(url.path, newContainer.appendingPathComponent("Documents").path)
      started.fulfill()
    }
    wait(for: [started], timeout: 5)
  }

  /// A monitor with an available account and a container of its own.
  private func makeMonitor(with fileManager: FileManagerMock) -> iCloudDocumentsMonitor {
    let containerUrl = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    fileManager.stubUbiquityIdentityToken = NSString(string: "mockToken")
    fileManager.stubCloudDirectory = containerUrl
    let monitor = iCloudDocumentsMonitor(fileManager: fileManager,
                                         cloudContainerIdentifier: cloudContainerIdentifier,
                                         fileType: .kml)
    addTeardownBlock {
      monitor.stop()
      try? FileManager.default.removeItem(at: containerUrl)
    }
    return monitor
  }
}
