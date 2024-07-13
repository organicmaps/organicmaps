import XCTest
@testable import Organic_Maps__Debug_

class LocalDirectoryMonitorDelegateMock: LocalDirectoryMonitorDelegate {
  var contents = LocalContentsMetadata()

  var didFinishGatheringExpectation: XCTestExpectation?
  var didUpdateExpectation: XCTestExpectation?
  var didReceiveErrorExpectation: XCTestExpectation?

  func didFinishGathering(contents: LocalContentsMetadata) {
    self.contents = contents
    didFinishGatheringExpectation?.fulfill()
  }

  func didUpdate(contents: LocalContentsMetadata) {
    self.contents = contents
    didUpdateExpectation?.fulfill()
  }

  func didReceiveLocalMonitorError(_ error: Error) {
    didReceiveErrorExpectation?.fulfill()
  }
}
