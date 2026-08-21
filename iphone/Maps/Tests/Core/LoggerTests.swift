@testable import Organic_Maps__Debug_
import XCTest

final class LoggerTests: XCTestCase {
  private var wasFileLoggingEnabled = false

  override func setUp() {
    super.setUp()
    wasFileLoggingEnabled = Logger.fileLoggingEnabled
  }

  override func tearDown() {
    Logger.fileLoggingEnabled = wasFileLoggingEnabled
    super.tearDown()
  }

  func testExportedArchiveIsCreatedOffTheMainThread() {
    Logger.fileLoggingEnabled = true
    XCTAssertTrue(Logger.fileLoggingEnabled)
    LOG(.info, "A record that has to reach the exported archive")

    var completedOnMainThread = true
    var archiveData: Data?
    let exported = expectation(description: "The log archive is created")
    Logger.getLogArchive { data in
      completedOnMainThread = Thread.isMainThread
      archiveData = data
      exported.fulfill()
    }
    wait(for: [exported], timeout: 30)

    XCTAssertFalse(completedOnMainThread, "Reading and zipping the log must not run on the main thread")
    XCTAssertGreaterThan(archiveData?.count ?? 0, 0)
  }

  func testEnablingCreatesTheLogAndDisablingRemovesIt() {
    Logger.fileLoggingEnabled = true
    LOG(.info, "A record that has to reach the log file")
    drainPendingWrites()
    XCTAssertGreaterThan(Logger.getLogFileSize(), 0)

    // Disabling runs synchronously on the same serial queue, so it also drains the asynchronous write.
    Logger.fileLoggingEnabled = false

    XCTAssertFalse(Logger.fileLoggingEnabled)
    XCTAssertEqual(Logger.getLogFileSize(), 0, "Disabling logging must remove every log file")
  }

  func testNoDiagnosticLogIsLeftInDocuments() throws {
    Logger.fileLoggingEnabled = true
    LOG(.info, "A record that has to reach the log file")

    // Documents is exposed to the user through the Files app and is included in backups.
    let documents = try FileManager.default.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: false)
    XCTAssertFalse(FileManager.default.fileExists(atPath: documents.appendingPathComponent("log.txt").path))
  }

  /// Ordinary records are written asynchronously. A completion enqueued on the same serial queue
  /// waits for every write that was submitted before it.
  private func drainPendingWrites() {
    let drained = expectation(description: "The pending writes are drained")
    Logger.flush { drained.fulfill() }
    wait(for: [drained], timeout: 30)
  }
}
