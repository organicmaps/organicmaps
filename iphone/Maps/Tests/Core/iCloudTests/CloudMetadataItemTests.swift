@testable import Organic_Maps__Debug_
import XCTest

final class CloudMetadataItemTests: XCTestCase {
  private let downloadingError = NSError(domain: "download", code: 7)
  private let uploadingError = NSError(domain: "upload", code: 8)
  private let validValues: [String: Any] = [
    NSMetadataItemFSNameKey: NSString(string: "Bookmarks.kml"),
    NSMetadataItemURLKey: NSURL(fileURLWithPath: "/tmp/Folder/../Bookmarks.kml"),
    NSMetadataUbiquitousItemDownloadingStatusKey: NSString(string: NSMetadataUbiquitousItemDownloadingStatusCurrent),
    NSMetadataUbiquitousItemPercentDownloadedKey: NSNumber(value: 100),
    NSMetadataItemFSContentChangeDateKey: NSDate(timeIntervalSince1970: 123.75),
    NSMetadataUbiquitousItemHasUnresolvedConflictsKey: NSNumber(value: false),
    NSMetadataUbiquitousItemDownloadingErrorKey: NSError(domain: "download", code: 7),
    NSMetadataUbiquitousItemUploadingErrorKey: NSError(domain: "upload", code: 8),
  ]

  func testBridgedValuesAreMappedToTheItem() throws {
    let result = CloudMetadataItem.make(from: validValues)
    let item = try XCTUnwrap(result.item)
    XCTAssertEqual(item.fileName, "Bookmarks.kml")
    XCTAssertEqual(item.fileUrl, URL(fileURLWithPath: "/tmp/Bookmarks.kml"))
    XCTAssertTrue(item.isDownloaded)
    XCTAssertEqual(item.percentDownloaded, 100)
    XCTAssertEqual(item.lastModificationDate, 123)
    XCTAssertEqual(item.downloadingError?.domain, downloadingError.domain)
    XCTAssertEqual(item.downloadingError?.code, downloadingError.code)
    XCTAssertEqual(item.uploadingError?.domain, uploadingError.domain)
    XCTAssertEqual(item.uploadingError?.code, uploadingError.code)
    XCTAssertFalse(item.hasUnresolvedConflicts)
    XCTAssertEqual(result.invalid, [])
  }

  func testMissingAttributeIsReported() {
    var values = validValues
    values.removeValue(forKey: NSMetadataItemURLKey)
    XCTAssertEqual(CloudMetadataItem.make(from: values).invalid,
                   ["\(NSMetadataItemURLKey) (missing)"])
  }

  func testAttributeOfUnexpectedTypeIsReported() {
    var values = validValues
    // The system reports the URL as a plain string instead of a URL.
    values[NSMetadataItemURLKey] = "/tmp/Bookmarks.kml"
    let invalid = CloudMetadataItem.make(from: values).invalid
    XCTAssertEqual(invalid.count, 1)
    XCTAssertTrue(invalid[0].contains(NSMetadataItemURLKey))
  }

  func testEveryMissingAttributeIsReported() {
    let invalid = CloudMetadataItem.make(from: [:]).invalid
    XCTAssertEqual(invalid.count, CloudMetadataItem.requiredAttributes.count)
    for key in CloudMetadataItem.requiredAttributes {
      XCTAssertTrue(invalid.contains("\(key) (missing)"))
    }
  }

  func testSynchronizationDescriptionContainsDecisionState() {
    let item = CloudMetadataItem(fileName: "Bookmarks.kml",
                                 fileUrl: URL(fileURLWithPath: "/tmp/.Trash/Bookmarks.kml"),
                                 isDownloaded: false,
                                 percentDownloaded: 42,
                                 lastModificationDate: 10,
                                 downloadingError: NSError(domain: "download", code: 7),
                                 uploadingError: nil,
                                 hasUnresolvedConflicts: true)

    let description = item.synchronizationDebugDescription
    XCTAssertTrue(description.contains("/tmp/.Trash/Bookmarks.kml"))
    XCTAssertTrue(description.contains("downloaded: false"))
    XCTAssertTrue(description.contains("percentDownloaded: 42"))
    XCTAssertTrue(description.contains("unresolvedConflicts: true"))
    XCTAssertTrue(description.contains("downloadingError: download#7"))
    XCTAssertFalse(description.contains("fileName:"))
  }
}
