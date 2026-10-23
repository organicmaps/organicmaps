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

  func testReloadRejectsAnEditMadeAfterItsRequest() throws {
    let bookmarks = BookmarksManager.shared()
    let name = "SyncReload-\(UUID().uuidString)"
    let (categoryId, localUrl) = try categoryFixture(named: name)
    let update = try XCTUnwrap(bookmarks.beginFileUpdate(atPath: localUrl.path))
    let incomingUrl = FileManager.default.temporaryDirectory.appendingPathComponent(name).appendingPathExtension("kml")
    try incomingContents(named: name).write(to: incomingUrl)
    defer { try? FileManager.default.removeItem(at: incomingUrl) }
    let reloaded = expectation(description: "Reload rejects an edit made after the request")
    bookmarks.reloadCategory(with: update, fromFilePath: incomingUrl.path) { applied in
      XCTAssertFalse(applied)
      reloaded.fulfill()
    }
    // The main queue cannot apply the asynchronous read before this edit is made.
    bookmarks.setCategory(categoryId, description: "edit after reload request")
    wait(for: [reloaded], timeout: 5)
    waitForSave(at: localUrl)

    XCTAssertEqual(bookmarks.category(withId: categoryId).detailedAnnotation, "edit after reload request")
    let saved = try Data(contentsOf: localUrl)
    XCTAssertTrue(String(decoding: saved, as: UTF8.self).contains("edit after reload request"))
  }

  func testPreservedCopyLoadingKeepsEditsToAnAlreadyLoadedCopy() throws {
    let bookmarks = BookmarksManager.shared()
    let name = "Preserved-\(UUID().uuidString)"
    let (categoryId, url) = try categoryFixture(named: name)

    bookmarks.loadCategoryIfNeeded(atFilePath: url.path)
    bookmarks.setCategory(categoryId, description: "edited preserved copy")
    waitForSave(at: url)

    XCTAssertEqual(bookmarks.category(withId: categoryId).detailedAnnotation, "edited preserved copy")
  }

  private func categoryFixture(named name: String) throws -> (MWMMarkGroupID, URL) {
    let bookmarks = BookmarksManager.shared()
    let categoryId = bookmarks.createCategory(withName: name)
    let url = FileManager.default.bookmarksDirectoryUrl.appendingPathComponent(name).appendingPathExtension("kml")
    addTeardownBlock {
      for category in bookmarks.sortedUserCategories() where category.title.hasPrefix(name) {
        bookmarks.deleteCategory(category.categoryId)
      }
      let trashed = bookmarks.getRecentlyDeletedCategories().filter { $0.title.hasPrefix(name) }.map(\.fileURL)
      bookmarks.deleteRecentlyDeletedCategory(at: trashed)
    }
    bookmarks.setCategory(categoryId, description: "original")
    waitForSave(at: url)
    return (categoryId, url)
  }

  private func waitForSave(at url: URL) {
    let saved = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      let bookmarks = BookmarksManager.shared()
      return bookmarks.areBookmarksLoaded() && FileManager.default.fileExists(atPath: url.path)
        && !bookmarks.isCategoryFileSaving(atPath: url.path)
    }, object: nil)
    wait(for: [saved], timeout: 5)
  }

  private func incomingContents(named name: String) -> Data {
    Data("""
    <?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2"><Document>
    <name>\(name) incoming</name><description>remote</description>
    <Placemark><name>Remote place</name><Point><coordinates>0,0</coordinates></Point></Placemark>
    </Document></kml>
    """.utf8)
  }
}
