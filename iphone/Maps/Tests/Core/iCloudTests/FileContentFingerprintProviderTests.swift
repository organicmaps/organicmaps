@testable import Organic_Maps__Debug_
import XCTest

final class FileContentFingerprintProviderTests: XCTestCase {
  private var directoryUrl: URL!
  private var provider: FileContentFingerprintProvider!

  override func setUp() {
    super.setUp()
    directoryUrl = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    try? FileManager.default.createDirectory(at: directoryUrl, withIntermediateDirectories: true)
    provider = FileContentFingerprintProvider(failedReadRetryInterval: 0.05)
  }

  override func tearDown() {
    provider = nil
    try? FileManager.default.removeItem(at: directoryUrl)
    directoryUrl = nil
    super.tearDown()
  }

  func testContentIsHashedInTheBackground() throws {
    let content = "content"
    let item = try file(named: "file.kml", content: content)
    let reported = expectation(description: "The content is known")
    provider.onContentsMayBeKnown = { reported.fulfill() }

    XCTAssertNil(provider.fingerprint(of: item), "Reading and hashing a file is too slow for the main queue")
    wait(for: [reported], timeout: 1)
    XCTAssertEqual(provider.fingerprint(of: item), Fingerprint(hashing: Data(content.utf8)))
  }

  /// A file evicted by iCloud, or replaced while it was being read, cannot be read at all. Nothing reports that
  /// it became readable, so giving up on it would leave it unsynchronized for the rest of the session.
  func testFileThatCannotBeReadIsTriedAgain() {
    let item = item(named: "missing.kml")

    let firstReport = expectation(description: "The file may be worth reading again")
    provider.onContentsMayBeKnown = { firstReport.fulfill() }
    XCTAssertNil(provider.fingerprint(of: item))
    wait(for: [firstReport], timeout: 1)

    let secondReport = expectation(description: "The file may be worth reading again")
    provider.onContentsMayBeKnown = { secondReport.fulfill() }
    XCTAssertNil(provider.fingerprint(of: item), "The file still cannot be read")
    wait(for: [secondReport], timeout: 1)
  }

  // MARK: - Helpers

  private func item(named fileName: String) -> LocalMetadataItem {
    LocalMetadataItem(fileName: fileName,
                      fileUrl: directoryUrl.appendingPathComponent(fileName),
                      lastModificationDate: 1,
                      size: 0)
  }

  private func file(named fileName: String, content: String) throws -> LocalMetadataItem {
    let item = item(named: fileName)
    try Data(content.utf8).write(to: item.fileUrl)
    return item
  }
}
