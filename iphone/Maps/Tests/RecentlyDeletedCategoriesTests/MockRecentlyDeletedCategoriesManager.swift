@testable import Organic_Maps__Debug_

class MockRecentlyDeletedCategoriesManager: NSObject, RecentlyDeletedCategoriesManager {
  func areRecentlyDeletedCategoriesEmpty() -> Bool {
    categories.isEmpty
  }
  
  var categories = [RecentlyDeletedCategory]()

  func getRecentlyDeletedCategories() -> [URL] {
    categories.map { $0.fileURL }
  }

  func deleteRecentlyDeletedCategory(at urls: [URL]) {
    categories.removeAll { urls.contains($0.fileURL) }
  }

  func deleteAllRecentlyDeletedCategories() {
    categories.removeAll()
  }

  func recoverRecentlyDeletedCategories(at urls: [URL]) {
    categories.removeAll { urls.contains($0.fileURL) }
  }
}
