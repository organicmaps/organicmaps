@testable import Organic_Maps__Debug_
import XCTest

final class BookmarkFileOperationsTests: XCTestCase {
  func testConfirmedDeletionTrashesAFileThatHasNoLoadedCategory() throws {
    let bookmarksManager = BookmarksManager.shared()
    XCTAssertTrue(bookmarksManager.areBookmarksLoaded())
    let name = "UnparsedSyncTests-\(UUID().uuidString).kml"
    let fileUrl = FileManager.default.bookmarksDirectoryUrl.appendingPathComponent(name)
    let trashUrl = fileUrl.deletingLastPathComponent().deletingLastPathComponent()
      .appendingPathComponent(".Trash").appendingPathComponent(name)
    let data = Data("unsupported KML content".utf8)
    try data.write(to: fileUrl)
    addTeardownBlock {
      try? FileManager.default.removeItem(at: fileUrl)
      try? FileManager.default.removeItem(at: trashUrl)
    }

    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path))

    XCTAssertFalse(FileManager.default.fileExists(atPath: fileUrl.path))
    XCTAssertEqual(try Data(contentsOf: trashUrl), data)
    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path), "An absent file is already deleted")
  }
}
