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
    var archiveError: Error?
    let exported = expectation(description: "The log archive is created")
    Logger.getLogArchive { data, error in
      completedOnMainThread = Thread.isMainThread
      archiveData = data
      archiveError = error
      exported.fulfill()
    }
    wait(for: [exported], timeout: 30)

    XCTAssertFalse(completedOnMainThread, "Reading and zipping the log must not run on the main thread")
    XCTAssertNil(archiveError)
    XCTAssertGreaterThan(archiveData?.count ?? 0, 0)
    let names = archiveData.map(archiveEntryNames) ?? []
    XCTAssertTrue(names.contains("log.txt"))
    XCTAssertTrue(names.contains("system-log.txt"))
  }

  func testArchiveIsRebuiltFromTheSystemLogWhenFileLoggingIsOff() {
    Logger.fileLoggingEnabled = false
    LOG(.info, "A record that has to reach the system log report")

    var archiveData: Data?
    var archiveError: Error?
    let exported = expectation(description: "The system log archive is created")
    Logger.getLogArchive { data, error in
      archiveData = data
      archiveError = error
      exported.fulfill()
    }
    wait(for: [exported], timeout: 30)

    XCTAssertNil(archiveError)
    XCTAssertGreaterThan(archiveData?.count ?? 0, 0)
    XCTAssertEqual(archiveData.map(archiveEntryNames), ["system-log.txt"])
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

  /// Reads file names from ZIP central-directory records. The archive payload stays compressed.
  private func archiveEntryNames(_ data: Data) -> Set<String> {
    let bytes = [UInt8](data)
    let centralDirectorySignature: UInt32 = 0x0201_4B50
    var names = Set<String>()
    var offset = 0
    while offset + 46 <= bytes.count {
      if uint32(bytes, at: offset) != centralDirectorySignature {
        offset += 1
        continue
      }

      let nameLength = uint16(bytes, at: offset + 28)
      let extraLength = uint16(bytes, at: offset + 30)
      let commentLength = uint16(bytes, at: offset + 32)
      let nameStart = offset + 46
      let nameEnd = nameStart + nameLength
      guard nameEnd <= bytes.count else { break }
      if let name = String(bytes: bytes[nameStart ..< nameEnd], encoding: .utf8) {
        names.insert(name)
      }
      offset = nameEnd + extraLength + commentLength
    }
    return names
  }

  private func uint16(_ bytes: [UInt8], at offset: Int) -> Int {
    Int(bytes[offset]) | Int(bytes[offset + 1]) << 8
  }

  private func uint32(_ bytes: [UInt8], at offset: Int) -> UInt32 {
    UInt32(bytes[offset]) |
      UInt32(bytes[offset + 1]) << 8 |
      UInt32(bytes[offset + 2]) << 16 |
      UInt32(bytes[offset + 3]) << 24
  }
}
