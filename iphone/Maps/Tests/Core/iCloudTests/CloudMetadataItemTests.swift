@testable import Organic_Maps__Debug_
import XCTest

final class CloudMetadataItemTests: XCTestCase {
  private let validValues: [String: Any] = [
    NSMetadataItemFSNameKey: "Bookmarks.kml",
    NSMetadataItemURLKey: URL(fileURLWithPath: "/tmp/Bookmarks.kml"),
    NSMetadataUbiquitousItemDownloadingStatusKey: NSMetadataUbiquitousItemDownloadingStatusCurrent,
    NSMetadataUbiquitousItemPercentDownloadedKey: NSNumber(value: 100),
    NSMetadataItemFSContentChangeDateKey: Date(),
    NSMetadataUbiquitousItemHasUnresolvedConflictsKey: false,
  ]

  func testValidValuesHaveNoInvalidAttributes() {
    XCTAssertEqual(CloudMetadataItem.invalidAttributes(in: validValues), [])
  }

  func testMissingAttributeIsReported() {
    var values = validValues
    values.removeValue(forKey: NSMetadataItemURLKey)
    let invalid = CloudMetadataItem.invalidAttributes(in: values)
    XCTAssertEqual(invalid.count, 1)
    XCTAssertTrue(invalid[0].contains(NSMetadataItemURLKey))
    XCTAssertTrue(invalid[0].contains("missing"))
  }

  func testAttributeOfUnexpectedTypeIsReported() {
    var values = validValues
    // The system reports the URL as a plain string instead of a URL.
    values[NSMetadataItemURLKey] = "/tmp/Bookmarks.kml"
    let invalid = CloudMetadataItem.invalidAttributes(in: values)
    XCTAssertEqual(invalid.count, 1)
    XCTAssertTrue(invalid[0].contains(NSMetadataItemURLKey))
  }

  func testEveryMissingAttributeIsReported() {
    let invalid = CloudMetadataItem.invalidAttributes(in: [:])
    XCTAssertEqual(invalid.count, validValues.count)
    for key in validValues.keys {
      XCTAssertTrue(invalid.contains("\(key) (missing)"))
    }
  }
}
