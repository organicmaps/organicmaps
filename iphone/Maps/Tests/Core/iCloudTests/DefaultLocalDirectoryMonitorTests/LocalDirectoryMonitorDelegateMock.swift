import XCTest
@testable import Organic_Maps__Debug_

class LocalDirectoryMonitorDelegateMock: LocalDirectoryMonitorDelegate {
  var contents = LocalContents()

  var didFinishGatheringExpectation: XCTestExpectation?
  var didUpdateExpectation: XCTestExpectation?
  var didReceiveErrorExpectation: XCTestExpectation?

  func didFinishGathering(contents: LocalContents) {
    self.contents = contents
    didFinishGatheringExpectation?.fulfill()
  }

  func didUpdate(contents: LocalContents) {
    self.contents = contents
    didUpdateExpectation?.fulfill()
  }

  func didReceiveLocalMonitorError(_ error: Error) {
    didReceiveErrorExpectation?.fulfill()
  }
}
