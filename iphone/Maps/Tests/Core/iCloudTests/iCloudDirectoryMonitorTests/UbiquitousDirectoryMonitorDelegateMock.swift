@testable import Organic_Maps__Debug_
import XCTest

class UbiquitousDirectoryMonitorDelegateMock: CloudDirectoryMonitorDelegate {
  var didFinishGatheringCalled = false
  var didUpdateCalled = false
  var didReceiveErrorCalled = false

  var didFinishGatheringExpectation: XCTestExpectation?
  var didUpdateExpectation: XCTestExpectation?
  var didReceiveErrorExpectation: XCTestExpectation?

  var contents = CloudContents()

  func didFinishGathering(_ contents: CloudContents) {
    didFinishGatheringCalled = true
    didFinishGatheringExpectation?.fulfill()
    self.contents = contents
  }

  func didUpdate(_ contents: CloudContents, _: CloudContentsUpdate) {
    didUpdateCalled = true
    didUpdateExpectation?.fulfill()
    self.contents = contents
  }

  func didReceiveCloudMonitorError(_: Error) {
    didReceiveErrorCalled = true
    didReceiveErrorExpectation?.fulfill()
  }
}
