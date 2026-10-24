@testable import Organic_Maps__Debug_
import XCTest

class UbiquitousDirectoryMonitorDelegateMock: CloudDirectoryMonitorDelegate {
  var contents = CloudContents()

  var didReceiveSnapshotExpectation: XCTestExpectation?
  var didReceiveErrorExpectation: XCTestExpectation?

  func didReceiveCloudSnapshot(_ snapshot: CloudSnapshot) {
    contents = snapshot.items
    didReceiveSnapshotExpectation?.fulfill()
  }

  func didReceiveCloudMonitorError(_: Error) {
    didReceiveErrorExpectation?.fulfill()
  }
}
