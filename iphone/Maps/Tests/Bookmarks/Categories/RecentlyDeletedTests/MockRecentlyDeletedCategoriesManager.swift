class MockRecentlyDeletedCategoriesManager: NSObject, RecentlyDeletedCategoriesManager, BookmarksObservable {
  var categories = [RecentlyDeletedCategory]()

  func hasRecentlyDeletedCategories() -> Bool {
    !categories.isEmpty
  }

  func getRecentlyDeletedCategories() -> [RecentlyDeletedCategory] {
    categories
  }

  func deleteFile(at urls: [URL]) {
    categories.removeAll { urls.contains($0.fileURL) }
  }

  func deleteAllRecentlyDeletedCategories() {
    categories.removeAll()
  }

  func recoverRecentlyDeletedCategories(at urls: [URL]) {
    categories.removeAll { urls.contains($0.fileURL) }
  }

  func deleteRecentlyDeletedCategory(at urls: [URL]) {
    categories.removeAll { urls.contains($0.fileURL) }
  }

  func add(_ observer: any BookmarksObserver) {}

  func remove(_ observer: any BookmarksObserver) {}
}
