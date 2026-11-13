@testable import Organic_Maps__Debug_
import XCTest

final class BookmarkFileOperationsTests: XCTestCase {
  func testUnparsedFileCanBeTrashedRecoveredAndDeleted() throws {
    let bookmarksManager = BookmarksManager.shared()
    waitForBookmarks(bookmarksManager)
    let name = "UnparsedSyncTests-\(UUID().uuidString).kml"
    let fileUrl = FileManager.default.bookmarksDirectoryUrl.appendingPathComponent(name)
    let trashUrl = fileUrl.deletingLastPathComponent().deletingLastPathComponent()
      .appendingPathComponent(".Trash").appendingPathComponent(name)
    let data = Data("unsupported KML content".utf8)
    try FileManager.default.createDirectory(at: fileUrl.deletingLastPathComponent(), withIntermediateDirectories: true)
    try data.write(to: fileUrl)
    addTeardownBlock {
      try? FileManager.default.removeItem(at: fileUrl)
      try? FileManager.default.removeItem(at: trashUrl)
    }

    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path))

    XCTAssertFalse(FileManager.default.fileExists(atPath: fileUrl.path))
    XCTAssertEqual(try Data(contentsOf: trashUrl), data)
    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path), "An absent file is already deleted")

    let category = try XCTUnwrap(bookmarksManager.getRecentlyDeletedCategories().first { $0.fileURL == trashUrl })
    XCTAssertEqual(category.title, fileUrl.deletingPathExtension().lastPathComponent)
    bookmarksManager.recoverRecentlyDeletedCategories(at: [trashUrl])
    waitForBookmarks(bookmarksManager)
    XCTAssertEqual(try Data(contentsOf: fileUrl), data)
    XCTAssertFalse(FileManager.default.fileExists(atPath: trashUrl.path))

    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path))
    bookmarksManager.deleteRecentlyDeletedCategory(at: [trashUrl])
    XCTAssertFalse(FileManager.default.fileExists(atPath: trashUrl.path))
  }

  private func waitForBookmarks(_ manager: BookmarksManager) {
    let loaded = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in manager.areBookmarksLoaded() }, object: nil)
    wait(for: [loaded], timeout: 5)
  }
}
