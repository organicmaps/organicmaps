import CarPlay
import ObjectiveC
@testable import Organic_Maps__Debug_
import XCTest

final class CarPlayBookmarkCategoryTests: XCTestCase {
  private static var categories = NSArray()

  func testCategoryRowsRespectTheCarPlayDisplayLimit() throws {
    precondition(Thread.isMainThread)
    // Keep fixture categories independent of the app's persisted bookmark lists.
    let method = try XCTUnwrap(class_getInstanceMethod(BookmarksManager.self, #selector(BookmarksManager.sortedUserCategories)))
    let supplier: @convention(c) (AnyObject, Selector) -> Unmanaged<NSArray> = { _, _ in
      .passUnretained(CarPlayBookmarkCategoryTests.categories)
    }
    let original = method_setImplementation(method, unsafeBitCast(supplier, to: IMP.self))
    defer {
      method_setImplementation(method, original)
      Self.categories = NSArray()
    }

    let limit = CPListTemplate.maximumItemCount
    for count in [0, limit - 1, limit, limit + 1] {
      let categories = (0 ..< count + 2).map {
        MockBookmarkGroup(categoryId: UInt64($0 + 1), bookmarksManager: BookmarksManager.shared())
      }
      categories.first?.markCount = 0
      categories.last?.markCount = 0
      Self.categories = categories as NSArray

      let template = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
      let rows = template.sections.flatMap(\.items).compactMap { $0 as? CPListItem }
      let cropped = count > limit
      let visibleCount = cropped ? limit - 1 : count
      XCTAssertEqual(rows.filter(\.isEnabled).compactMap(\.text), (0 ..< visibleCount).map { "Category \($0 + 2)" }, "count=\(count)")
      XCTAssertEqual(rows.contains { !$0.isEnabled }, cropped, "count=\(count)")
      XCTAssertLessThanOrEqual(rows.count, limit)
    }
  }

  private final class MockBookmarkGroup: BookmarkGroup {
    var markCount = 1

    override var title: String { "Category \(categoryId)" }
    override var bookmarksCount: Int { markCount }
    override var trackCount: Int { 0 }
  }
}
