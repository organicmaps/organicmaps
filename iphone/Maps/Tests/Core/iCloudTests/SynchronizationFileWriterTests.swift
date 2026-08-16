@testable import Organic_Maps__Debug_
import XCTest

/// The writer works on two plain directories here: the operations it performs, and the file coordination they
/// run under, do not depend on the cloud directory being an iCloud container.
final class SynchronizationFileWriterTests: XCTestCase {
  private var rootUrl: URL!
  private var localDirectoryUrl: URL!
  private var cloudDirectoryUrl: URL!
  private var writer: SynchronizationFileWriter!

  override func setUp() {
    super.setUp()
    rootUrl = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    localDirectoryUrl = rootUrl.appendingPathComponent("local")
    cloudDirectoryUrl = rootUrl.appendingPathComponent("cloud")
    try? FileManager.default.createDirectory(at: localDirectoryUrl, withIntermediateDirectories: true)
    try? FileManager.default.createDirectory(at: cloudDirectoryUrl, withIntermediateDirectories: true)
    writer = SynchronizationFileWriter(fileManager: .default,
                                       localDirectoryUrl: localDirectoryUrl,
                                       cloudDirectoryUrl: cloudDirectoryUrl)
  }

  override func tearDown() {
    writer = nil
    try? FileManager.default.removeItem(at: rootUrl)
    rootUrl = nil
    localDirectoryUrl = nil
    cloudDirectoryUrl = nil
    super.tearDown()
  }

  // MARK: - A file is replaced only while it holds the content the decision was made from

  func testLocalFileHoldingTheDecidedContentIsReplaced() throws {
    let cloudItem = try cloudFile("file.kml", content: "B")
    let localItem = try localFile("file.kml", content: "A")

    let result = try process(.updateLocalItem(with: cloudItem, replacing: fingerprint("A"), preservingLocal: false))

    guard case .reloadCategoriesAtURLs(let urls) = result else { return XCTFail("Unexpected result: \(result)") }
    XCTAssertEqual(urls.map(path), [path(localItem.fileUrl)])
    XCTAssertEqual(content(of: localItem.fileUrl), "B")
  }

  /// The app saves a category without coordinating it, so the local file may hold a version that was written
  /// after the decision. Nobody has compared it with the cloud copy yet and it is the only copy of itself.
  func testLocalFileChangedSinceTheDecisionIsNotReplaced() throws {
    let cloudItem = try cloudFile("file.kml", content: "B")
    let localItem = try localFile("file.kml", content: "C")

    let result = try process(.updateLocalItem(with: cloudItem, replacing: fingerprint("A"), preservingLocal: false))

    assertSkipped(result)
    XCTAssertEqual(content(of: localItem.fileUrl), "C")
  }

  func testReplacedLocalVersionIsKeptUnderANewName() throws {
    let cloudItem = try cloudFile("file.kml", content: "B")
    let localItem = try localFile("file.kml", content: "A")

    let result = try process(.updateLocalItem(with: cloudItem, replacing: fingerprint("A"), preservingLocal: true))

    guard case .reloadCategoriesAtURLs(let urls) = result, urls.count == 2 else {
      return XCTFail("The preserved copy and the replaced file both have to be loaded, got \(result)")
    }
    XCTAssertEqual(path(urls[0]), path(localDirectoryUrl.appendingPathComponent(Self.copyHoldingA)))
    XCTAssertEqual(content(of: urls[0]), "A")
    XCTAssertEqual(path(urls[1]), path(localItem.fileUrl))
    XCTAssertEqual(content(of: localItem.fileUrl), "B")
  }

  /// The same version is kept once: another device, or an earlier conflict of the same file, wrote this very
  /// copy already. It is not written again and is loaded together with the file all the same.
  func testPreservedCopyThatIsThereAlreadyIsNotWrittenAgain() throws {
    let cloudItem = try cloudFile("file.kml", content: "B")
    let localItem = try localFile("file.kml", content: "A")
    let copyUrl = localDirectoryUrl.appendingPathComponent(Self.copyHoldingA)
    try Data("A".utf8).write(to: copyUrl)

    let result = try process(.updateLocalItem(with: cloudItem, replacing: fingerprint("A"), preservingLocal: true))

    guard case .reloadCategoriesAtURLs(let urls) = result else { return XCTFail("Unexpected result: \(result)") }
    XCTAssertEqual(urls.map(path), [path(copyUrl), path(localItem.fileUrl)])
    XCTAssertEqual(content(of: copyUrl), "A")
    XCTAssertEqual(content(of: localItem.fileUrl), "B")
  }

  /// A copy that was there already holds a version of its own: a write that fails must not take it away. The
  /// copy the same write made preserves nothing then -- the local file is still there -- and goes.
  func testFailedReplaceRemovesOnlyTheCopyItWrote() throws {
    writer = SynchronizationFileWriter(fileManager: UnreplaceableFilesFileManager(),
                                       localDirectoryUrl: localDirectoryUrl,
                                       cloudDirectoryUrl: cloudDirectoryUrl)
    let cloudItem = try cloudFile("file.kml", content: "B")
    let localItem = try localFile("file.kml", content: "A")
    let copyUrl = localDirectoryUrl.appendingPathComponent(Self.copyHoldingA)
    let update = OutgoingSynchronizationEvent.updateLocalItem(with: cloudItem,
                                                              replacing: fingerprint("A"),
                                                              preservingLocal: true)

    try assertFailed(process(update))
    XCTAssertFalse(FileManager.default.fileExists(atPath: copyUrl.path))
    XCTAssertEqual(content(of: localItem.fileUrl), "A")

    try Data("A".utf8).write(to: copyUrl)
    try assertFailed(process(update))
    XCTAssertEqual(content(of: copyUrl), "A")
  }

  func testCloudFileHoldingTheDecidedContentIsReplaced() throws {
    let localItem = try localFile("file.kml", content: "C")
    let cloudItem = try cloudFile("file.kml", content: "A")

    let result = try process(.updateCloudItem(with: localItem, replacing: fingerprint("A")))

    guard case .success = result else { return XCTFail("Unexpected result: \(result)") }
    XCTAssertEqual(content(of: cloudItem.fileUrl), "C")
  }

  /// A version another device uploaded in the meantime is no conflict for iCloud: overwriting it would install
  /// the local content over it on every device.
  func testCloudFileChangedSinceTheDecisionIsNotReplaced() throws {
    let localItem = try localFile("file.kml", content: "C")
    let cloudItem = try cloudFile("file.kml", content: "B")

    let result = try process(.updateCloudItem(with: localItem, replacing: fingerprint("A")))

    assertSkipped(result)
    XCTAssertEqual(content(of: cloudItem.fileUrl), "B")
  }

  func testFileThatIsBackIsNotCreatedOverAgain() throws {
    let cloudItem = try cloudFile("fromCloud.kml", content: "B")
    let localItem = try localFile("fromCloud.kml", content: "A")
    try assertSkipped(process(.createLocalItem(with: cloudItem)))
    XCTAssertEqual(content(of: localItem.fileUrl), "A")

    let uploadedItem = try localFile("fromLocal.kml", content: "A")
    let existingCloudItem = try cloudFile("fromLocal.kml", content: "B")
    try assertSkipped(process(.createCloudItem(with: uploadedItem)))
    XCTAssertEqual(content(of: existingCloudItem.fileUrl), "B")
  }

  /// A file another device uploaded is there before iCloud makes its content readable, and a file that cannot be
  /// read is not an absent one: what it holds has never been compared with anything.
  func testUnreadableFileIsNotCreatedOverAgain() throws {
    let cloudItem = try cloudFile("file.kml", content: "B")
    let localItem = try localFile("file.kml", content: "A")
    try makeUnreadable(localItem.fileUrl)

    try assertSkipped(process(.createLocalItem(with: cloudItem)))

    try makeReadable(localItem.fileUrl)
    XCTAssertEqual(content(of: localItem.fileUrl), "A")
  }

  /// The list was deleted right after it was edited: the upload has nothing to read and nothing is wrong.
  func testUploadOfAFileThatIsGoneIsSkipped() throws {
    let cloudItem = try cloudFile("file.kml", content: "A")

    let result = try process(.updateCloudItem(with: localItem("file.kml"), replacing: fingerprint("A")))

    assertSkipped(result)
    XCTAssertEqual(content(of: cloudItem.fileUrl), "A")
  }

  // MARK: - A file is deleted only while it holds the content that was synchronized

  func testLocalFileHoldingTheSynchronizedContentIsReportedForDeletion() throws {
    let localItem = try localFile("file.kml", content: "A")

    let result = try process(.removeLocalItem(localItem, evidence(content: "A")))

    guard case .deleteCategory(let url) = result else { return XCTFail("Unexpected result: \(result)") }
    XCTAssertEqual(path(url), path(localItem.fileUrl))
  }

  /// The file was saved again within the debounce of the local monitor: what it holds now was never compared
  /// with the copy that survives the deletion.
  func testLocalFileChangedSinceTheDeletionIsNotDeleted() throws {
    let localItem = try localFile("file.kml", content: "B")

    try assertSkipped(process(.removeLocalItem(localItem, evidence(content: "A"))))
    XCTAssertEqual(content(of: localItem.fileUrl), "B")
  }

  /// Another device changed the file while the deletion was crossing the queues: trashing is irreversible and
  /// the file it was decided for is not the one that is there. The trashing itself is not covered: the simulator
  /// has no volume with a trash to move a file to.
  func testCloudFileChangedSinceTheDeletionIsNotTrashed() throws {
    let cloudItem = try cloudFile("file.kml", content: "B")

    try assertSkipped(trash(cloudItem.fileUrl, expecting: fingerprint("A")))
    XCTAssertEqual(content(of: cloudItem.fileUrl), "B")
  }

  /// iCloud does not start a download while offline, in Low Data Mode, or when it is busy with the item.
  /// Nothing was written and nothing is wrong: the request is repeated on a later snapshot.
  func testDownloadThatCannotStartIsSkipped() throws {
    writer = SynchronizationFileWriter(fileManager: UndownloadableItemsFileManager(),
                                       localDirectoryUrl: localDirectoryUrl,
                                       cloudDirectoryUrl: cloudDirectoryUrl)

    try assertSkipped(process(.startDownloading(cloudItem("file.kml"))))
  }

  // MARK: - A version that is kept aside is named after the content it holds

  /// The name of a copy is decided by its content alone: every device that keeps this very version writes the
  /// same file next to the original one, and a file under that name holds this version already.
  func testConflictCopyIsNamedAfterTheContentItHolds() {
    let fileUrl = cloudDirectoryUrl.appendingPathComponent("file.kml")

    XCTAssertEqual(writer.copyUrl(of: fileUrl, holding: fingerprint("A")).lastPathComponent, Self.copyHoldingA)
    XCTAssertNotEqual(writer.copyUrl(of: fileUrl, holding: fingerprint("B")),
                      writer.copyUrl(of: fileUrl, holding: fingerprint("A")))
  }

  // MARK: - Helpers

  /// How a copy of "file.kml" holding "A" is named: the first six bytes of the SHA-256 of the content.
  private static let copyHoldingA = "file_559aead08264.kml"

  /// A busy machine can still delay an answer from the writer's utility queue. The generous timeout costs
  /// nothing: on an idle machine the result arrives at once.
  private static let resultTimeout: TimeInterval = 30

  private func process(_ event: OutgoingSynchronizationEvent) throws -> WritingResult {
    var result: WritingResult?
    let processed = expectation(description: "The event is processed")
    writer.processEvent(event) {
      result = $0
      processed.fulfill()
    }
    wait(for: [processed], timeout: Self.resultTimeout)
    return try XCTUnwrap(result)
  }

  private func trash(_ url: URL, expecting expectedContent: Fingerprint) throws -> WritingResult {
    var result: WritingResult?
    let processed = expectation(description: "The file is trashed")
    writer.trashCloudItem(at: url, expecting: expectedContent) {
      result = $0
      processed.fulfill()
    }
    wait(for: [processed], timeout: Self.resultTimeout)
    return try XCTUnwrap(result)
  }

  private func assertSkipped(_ result: WritingResult, file: StaticString = #filePath, line: UInt = #line) {
    guard case .skipped = result else {
      return XCTFail("Nothing must be written, got \(result)", file: file, line: line)
    }
  }

  private func assertFailed(_ result: WritingResult, file: StaticString = #filePath, line: UInt = #line) {
    guard case .failure = result else {
      return XCTFail("The write must fail, got \(result)", file: file, line: line)
    }
  }

  private func localItem(_ fileName: String) -> LocalMetadataItem {
    LocalMetadataItem(fileName: fileName,
                      fileUrl: localDirectoryUrl.appendingPathComponent(fileName),
                      lastModificationDate: 1,
                      size: 0)
  }

  private func cloudItem(_ fileName: String) -> CloudMetadataItem {
    CloudMetadataItem(fileName: fileName,
                      fileUrl: cloudDirectoryUrl.appendingPathComponent(fileName),
                      lastModificationDate: 1,
                      size: 0,
                      isDownloaded: true,
                      hasUnresolvedConflicts: false,
                      downloadingError: nil,
                      uploadingError: nil)
  }

  private func localFile(_ fileName: String, content: String) throws -> LocalMetadataItem {
    let item = localItem(fileName)
    try Data(content.utf8).write(to: item.fileUrl)
    return item
  }

  private func cloudFile(_ fileName: String, content: String) throws -> CloudMetadataItem {
    let item = cloudItem(fileName)
    try Data(content.utf8).write(to: item.fileUrl)
    return item
  }

  /// Denying every access is how a file that exists but cannot be read is made here: the real one is a copy
  /// iCloud has not downloaded yet.
  private func makeUnreadable(_ fileUrl: URL) throws {
    try FileManager.default.setAttributes([.posixPermissions: 0], ofItemAtPath: fileUrl.path)
  }

  private func makeReadable(_ fileUrl: URL) throws {
    try FileManager.default.setAttributes([.posixPermissions: 0o644], ofItemAtPath: fileUrl.path)
  }

  private func content(of fileUrl: URL) -> String? {
    (try? Data(contentsOf: fileUrl)).map { String(decoding: $0, as: UTF8.self) }
  }

  private func fingerprint(_ content: String) -> Fingerprint { Fingerprint(hashing: Data(content.utf8)) }

  /// The writer verifies the content of the file only: the absence behind a deletion is confirmed elsewhere.
  private func evidence(content: String) -> DeletionEvidence {
    DeletionEvidence(absentSince: 0, base: fingerprint(content))
  }

  /// File coordination reports the file it locked through the real path of the directory it is in.
  private func path(_ url: URL) -> String { url.resolvingSymlinksInPath().path }
}

/// A directory in which a file cannot be replaced: no space is left, or something else holds the file.
private final class UnreplaceableFilesFileManager: FileManager {
  override func replaceItem(at _: URL,
                            withItemAt _: URL,
                            backupItemName _: String?,
                            options _: FileManager.ItemReplacementOptions,
                            resultingItemURL _: AutoreleasingUnsafeMutablePointer<NSURL?>?) throws {
    throw CocoaError(.fileWriteOutOfSpace)
  }
}

/// An iCloud container that refuses to download the items it reports.
private final class UndownloadableItemsFileManager: FileManager {
  override func isUbiquitousItem(at _: URL) -> Bool { true }

  override func startDownloadingUbiquitousItem(at _: URL) throws {
    throw CocoaError(.ubiquitousFileUnavailable)
  }
}
