@testable import Organic_Maps__Debug_
import XCTest

class LocalDirectoryMonitorDelegateMock: LocalDirectoryMonitorDelegate {
  var contents = LocalContents()

  var didReceiveFirstSnapshotExpectation: XCTestExpectation?
  var didReceiveNextSnapshotExpectation: XCTestExpectation?
  var didReceiveErrorExpectation: XCTestExpectation?

  private var snapshotsCount = 0

  func didReceiveLocalSnapshot(_ snapshot: LocalSnapshot) {
    contents = snapshot.items
    snapshotsCount += 1
    if snapshotsCount == 1 {
      didReceiveFirstSnapshotExpectation?.fulfill()
    } else {
      didReceiveNextSnapshotExpectation?.fulfill()
    }
  }

  func didReceiveLocalMonitorError(_: Error) {
    didReceiveErrorExpectation?.fulfill()
  }
}
