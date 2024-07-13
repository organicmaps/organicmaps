import XCTest
@testable import Organic_Maps__Debug_

typealias UbiquityIdentityToken = NSCoding & NSCopying & NSObjectProtocol

class iCloudDirectoryMonitorTests: XCTestCase {

  var cloudMonitor: iCloudDocumentsDirectoryMonitor!
  var mockFileManager: FileManagerMock!
  var mockDelegate: UbiquitousDirectoryMonitorDelegateMock!
  var cloudContainerIdentifier: String = "iCloud.app.organicmaps.debug"

  override func setUp() {
    super.setUp()
    mockFileManager = FileManagerMock()
    mockDelegate = UbiquitousDirectoryMonitorDelegateMock()
    cloudMonitor = iCloudDocumentsDirectoryMonitor(fileManager: mockFileManager, cloudContainerIdentifier: cloudContainerIdentifier, fileType: .kml)
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

  func testStartWhenCloudAvailable() {
    mockFileManager.stubUbiquityIdentityToken = NSString(string: "mockToken")
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      if case .success = result {
        startExpectation.fulfill()
      }
    }
    waitForExpectations(timeout: 5)
    XCTAssertTrue(cloudMonitor.state == .started, "Monitor should be started when the cloud is available.")
  }

  func testStartWhenCloudNotAvailable() {
    mockFileManager.stubUbiquityIdentityToken = nil
    let startExpectation = expectation(description: "startExpectation")
    cloudMonitor.start { result in
      if case .failure(let error) = result, case SynchronizationError.iCloudIsNotAvailable = error {
        startExpectation.fulfill()
      }
    }
    waitForExpectations(timeout: 5)
    XCTAssertTrue(cloudMonitor.state == .stopped, "Monitor should not start when the cloud is not available.")
  }

  func testStopAfterStart() {
    testStartWhenCloudAvailable()
    cloudMonitor.stop()
    XCTAssertTrue(cloudMonitor.state == .stopped, "Monitor should not be started after stopping.")
  }

  func testPauseAndResume() {
    testStartWhenCloudAvailable()
    cloudMonitor.pause()
    XCTAssertTrue(cloudMonitor.state == .paused, "Monitor should be paused.")

    cloudMonitor.resume()
    XCTAssertTrue(cloudMonitor.state == .started, "Monitor should not be paused after resuming.")
  }

  func testFetchUbiquityDirectoryUrl() {
    let expectation = self.expectation(description: "Fetch Ubiquity Directory URL")
    mockFileManager.shouldReturnContainerURL = true
    cloudMonitor.fetchUbiquityDirectoryUrl { result in
      if case .success = result {
        expectation.fulfill()
      }
    }
    wait(for: [expectation], timeout: 5.0)
  }
}
