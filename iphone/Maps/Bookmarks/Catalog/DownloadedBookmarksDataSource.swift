class DownloadedBookmarksDataSource {

  private var categories: [MWMCatalogCategory] = []
  var categoriesCount: NSInteger {
    get {
      return categories.count
    }
  }

  init() {
    categories = MWMBookmarksManager.categoriesFromCatalog()
  }

  func category(at index: Int) -> MWMCatalogCategory {
    return categories[index]
  }
}
