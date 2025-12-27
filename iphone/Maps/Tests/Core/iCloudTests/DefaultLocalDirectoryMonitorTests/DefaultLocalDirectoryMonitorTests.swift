import XCTest
@testable import Organic_Maps__Debug_

final class DefaultLocalDirectoryMonitorTests: XCTestCase {

  let fileManager = FileManager.default
  let tempDirectory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
  var directoryMonitor: FileSystemDispatchSourceMonitor!
  var mockDelegate: LocalDirectoryMonitorDelegateMock!

  override func setUpWithError() throws {
    try super.setUpWithError()
    // Setup with a temporary directory and a mock delegate
    directoryMonitor = try FileSystemDispatchSourceMonitor(directory: tempDirectory, queue: .main)
    mockDelegate = LocalDirectoryMonitorDelegateMock()
    directoryMonitor.delegate = mockDelegate
  }

  override func tearDownWithError() throws {
    directoryMonitor.stop()
    mockDelegate = nil
    try? fileManager.removeItem(at: tempDirectory)
    try super.tearDownWithError()
  }

  func testInitialization() {
    XCTAssertEqual(directoryMonitor.directory, tempDirectory, "Monitor initialized with incorrect directory.")
    XCTAssertTrue(directoryMonitor.state == .stopped, "Monitor should be stopped initially.")
  }

  func testStartMonitoring() {
    let startExpectation = expectation(description: "Start monitoring")
    directoryMonitor.start { result in
      switch result {
      case .success:
        XCTAssertTrue(self.directoryMonitor.state == .started, "Monitor should be started.")
      case .failure(let error):
        XCTFail("Monitoring failed to start with error: \(error)")
      }
      startExpectation.fulfill()
    }
    wait(for: [startExpectation], timeout: 5.0)
  }

  func testStopMonitoring() {
    directoryMonitor.start()
    directoryMonitor.stop()
    XCTAssertTrue(directoryMonitor.state == .stopped, "Monitor should be stopped.")
  }

  func testDelegateDidFinishGathering() {
    mockDelegate.didFinishGatheringExpectation = expectation(description: "didFinishGathering called")
    directoryMonitor.start()
    wait(for: [mockDelegate.didFinishGatheringExpectation!], timeout: 5.0)
  }

  func testDelegateDidReceiveError() {
    mockDelegate.didReceiveErrorExpectation = expectation(description: "didReceiveLocalMonitorError called")

    let error = NSError(domain: NSCocoaErrorDomain, code: NSFileNoSuchFileError, userInfo: nil)
    directoryMonitor.delegate?.didReceiveLocalMonitorError(error)

    wait(for: [mockDelegate.didReceiveErrorExpectation!], timeout: 1.0)
  }
}
