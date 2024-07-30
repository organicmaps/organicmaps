import XCTest
@testable import Organic_Maps__Debug_

typealias UbiquityIdentityToken = NSCoding & NSCopying & NSObjectProtocol

class iCloudDirectoryMonitorTests: XCTestCase {

  var cloudMonitor: iCloudDocumentsDirectoryMonitor!
  var mockFileManager: FileManagerMock!
  var mockDelegate: UbiquitousDirectoryMonitorDelegateMock!
  var cloudContainerIdentifier: String = "iCloud.app.organicmaps.debug"
  var stubUbiquityIdentityToken = NSString(string: "token")

  override func setUp() {
    super.setUp()
    mockFileManager = FileManagerMock()
    mockDelegate = UbiquitousDirectoryMonitorDelegateMock()
    cloudMonitor = iCloudDocumentsDirectoryMonitor(fileManager: mockFileManager, cloudContainerIdentifier: cloudContainerIdentifier, fileType: .kml)
    cloudMonitor.delegate = mockDelegate
  }

  override func tearDown() {
    cloudMonitor.stop()
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
    mockFileManager.stubUbiquityIdentityToken = stubUbiquityIdentityToken
    XCTAssertTrue(cloudMonitor.isCloudAvailable())

    mockFileManager.stubUbiquityIdentityToken = nil
    XCTAssertFalse(cloudMonitor.isCloudAvailable())
  }

  func testStartWhenCloudAvailable() {
    mockFileManager.stubUbiquityIdentityToken = stubUbiquityIdentityToken
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      switch result {
      case .failure(let error):
        XCTFail("Failed to start monitor: \(error)")
      case .success:
        startExpectation.fulfill()
      }
    }
    wait(for: [startExpectation], timeout: 5)
    XCTAssertTrue(cloudMonitor.state == .started, "Monitor should be started when the cloud is available.")
  }

  func testStartWhenCloudNotAvailable() {
    mockFileManager.stubUbiquityIdentityToken = nil
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      switch result {
      case .failure(let error):
        if case SynchronizationError.iCloudIsNotAvailable = error {
          startExpectation.fulfill()
        } else {
          XCTFail("Incorrect error")
        }
      case .success:
        XCTFail("Monitor should not start when the cloud is not available.")
      }
    }
    wait(for: [startExpectation], timeout: 5)
    XCTAssertTrue(cloudMonitor.state == .stopped, "Monitor should not start when the cloud is not available.")
  }

  func testStopAfterStart() {
    mockFileManager.stubUbiquityIdentityToken = stubUbiquityIdentityToken
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      switch result {
      case .failure(let error):
        XCTFail("Failed to start monitor: \(error)")
      case .success:
        startExpectation.fulfill()
      }
    }
    wait(for: [startExpectation], timeout: 5)

    cloudMonitor.stop()
    XCTAssertTrue(cloudMonitor.state == .stopped, "Monitor should not be started after stopping.")
  }

  func testPauseAndResume() {
    mockFileManager.stubUbiquityIdentityToken = stubUbiquityIdentityToken
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      switch result {
      case .failure(let error):
        XCTFail("Failed to start monitor: \(error)")
      case .success:
        startExpectation.fulfill()
      }
    }
    wait(for: [startExpectation], timeout: 5)

    cloudMonitor.pause()
    XCTAssertTrue(cloudMonitor.state == .paused, "Monitor should be paused.")

    cloudMonitor.resume()
    XCTAssertTrue(cloudMonitor.state == .started, "Monitor should not be paused after resuming.")
  }

  func testFetchUbiquityDirectoryUrl() {
    let expectation = expectation(description: "Fetch Ubiquity Directory URL")
    cloudMonitor.fetchUbiquityDirectoryUrl { result in
      switch result {
      case .failure(let error):
        XCTFail("Failed to fetch ubiquity directory url: \(error)")
      case .success:
        expectation.fulfill()
      }
    }
    wait(for: [expectation], timeout: 5.0)
  }

  func testDelegateDidFinishGathering() {
    mockFileManager.stubUbiquityIdentityToken = stubUbiquityIdentityToken
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      switch result {
      case .failure(let error):
        XCTFail("Failed to start monitor: \(error)")
      case .success:
        startExpectation.fulfill()
      }
    }
    wait(for: [startExpectation], timeout: 5)

    guard let metadataQuery = cloudMonitor.metadataQuery else {
      XCTFail("Metadata query should not be nil")
      return
    }

    let didFinishGatheringExpectation = expectation(description: "didFinishGathering")
    mockDelegate.didFinishGatheringExpectation = didFinishGatheringExpectation
    NotificationCenter.default.post(name: .NSMetadataQueryDidFinishGathering, object: metadataQuery)
    wait(for: [didFinishGatheringExpectation], timeout: 5.0)
    XCTAssertTrue(mockDelegate.didFinishGatheringCalled, "Delegate's didFinishGathering should be called.")
  }

  func testDelegateDidUpdate() {
    mockFileManager.stubUbiquityIdentityToken = stubUbiquityIdentityToken
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      switch result {
      case .failure(let error):
        XCTFail("Failed to start monitor: \(error)")
      case .success:
        startExpectation.fulfill()
      }
    }
    wait(for: [startExpectation], timeout: 5)

    guard let metadataQuery = cloudMonitor.metadataQuery else {
      XCTFail("Metadata query should not be nil")
      return
    }
    let didUpdateExpectation = expectation(description: "didUpdate")
    mockDelegate.didUpdateExpectation = didUpdateExpectation
    NotificationCenter.default.post(name: NSNotification.Name.NSMetadataQueryDidUpdate, object: metadataQuery)
    wait(for: [didUpdateExpectation], timeout: 5.0)
    XCTAssertTrue(mockDelegate.didUpdateCalled, "Delegate's didUpdate should be called.")
  }

  func testDelegateMethods() {
    mockFileManager.stubUbiquityIdentityToken = stubUbiquityIdentityToken
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      switch result {
      case .failure(let error):
        XCTFail("Failed to start monitor: \(error)")
      case .success:
        startExpectation.fulfill()
      }
    }
    wait(for: [startExpectation], timeout: 5)

    guard let metadataQuery = cloudMonitor.metadataQuery else {
      XCTFail("Metadata query should not be nil")
      return
    }

    let didFinishGatheringExpectation = expectation(description: "didFinishGathering")
    mockDelegate.didFinishGatheringExpectation = didFinishGatheringExpectation
    NotificationCenter.default.post(name: NSNotification.Name.NSMetadataQueryDidFinishGathering, object: metadataQuery)
    wait(for: [didFinishGatheringExpectation], timeout: 5.0)

    let didUpdateExpectation = expectation(description: "didUpdate")
    mockDelegate.didUpdateExpectation = didUpdateExpectation
    NotificationCenter.default.post(name: NSNotification.Name.NSMetadataQueryDidUpdate, object: metadataQuery)
    wait(for: [didUpdateExpectation], timeout: 5.0)
  }
}
