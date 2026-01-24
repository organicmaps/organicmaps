@testable import Organic_Maps__Debug_
import XCTest

class LocalDirectoryMonitorDelegateMock: LocalDirectoryMonitorDelegate {
  var contents = LocalContents()

  var didFinishGatheringExpectation: XCTestExpectation?
  var didUpdateExpectation: XCTestExpectation?
  var didReceiveErrorExpectation: XCTestExpectation?

  func didFinishGathering(_ contents: LocalContents) {
    self.contents = contents
    didFinishGatheringExpectation?.fulfill()
  }

  func didUpdate(_ contents: LocalContents, _: LocalContentsUpdate) {
    self.contents = contents
    didUpdateExpectation?.fulfill()
  }

  func didReceiveLocalMonitorError(_: Error) {
    didReceiveErrorExpectation?.fulfill()
  }
}
