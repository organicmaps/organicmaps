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

  func testExportedArchiveIsCreatedOffTheMainThread() throws {
    Logger.fileLoggingEnabled = true
    XCTAssertTrue(Logger.fileLoggingEnabled)
    LOG(.info, "A record that has to reach the exported archive")

    var completedOnMainThread = true
    var archiveURL: URL?
    let exported = expectation(description: "The log archive is created")
    Logger.getLogFileURL { url in
      completedOnMainThread = Thread.isMainThread
      archiveURL = url
      exported.fulfill()
    }
    wait(for: [exported], timeout: 30)

    XCTAssertFalse(completedOnMainThread, "Reading and zipping the log must not run on the main thread")
    let url = try XCTUnwrap(archiveURL)
    XCTAssertEqual(url.pathExtension, "zip")
    let archiveSize = try XCTUnwrap(FileManager.default.attributesOfItem(atPath: url.path)[.size] as? UInt64)
    XCTAssertGreaterThan(archiveSize, 0)

    try FileManager.default.removeItem(at: url.deletingLastPathComponent())
  }

  func testEnablingCreatesTheLogAndDisablingRemovesIt() {
    Logger.fileLoggingEnabled = true
    LOG(.info, "A record that has to reach the log file")
    // Disabling is dispatched to the same serial queue, so it also drains the asynchronous write.
    Logger.fileLoggingEnabled = false

    XCTAssertFalse(Logger.fileLoggingEnabled)
    XCTAssertEqual(Logger.getLogFileSize(), 0, "Disabling logging must remove every log file")
  }

  func testNoDiagnosticLogIsLeftInDocuments() throws {
    Logger.fileLoggingEnabled = true
    LOG(.info, "A record that has to reach the log file")
    XCTAssertGreaterThan(Logger.getLogFileSize(), 0)

    // Documents is exposed to the user through the Files app and is included in backups.
    let documents = try FileManager.default.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: false)
    XCTAssertFalse(FileManager.default.fileExists(atPath: documents.appendingPathComponent("log.txt").path))
  }
}
