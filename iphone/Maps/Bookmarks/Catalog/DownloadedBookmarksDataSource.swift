class DownloadedBookmarksDataSource {

  private var categories: [MWMCatalogCategory] = []
  var categoriesCount: NSInteger {
    get {
      return categories.count
    }
  }

  var allCategoriesVisible: Bool {
    get {
      var result = true
      categories.forEach { if !$0.isVisible { result = false } }
      return result
    }
    set {
      categories.forEach {
        $0.isVisible = newValue
      }
      MWMBookmarksManager.setCatalogCategoriesVisible(newValue)
    }
  }

  init() {
    reload()
  }

  func category(at index: Int) -> MWMCatalogCategory {
    return categories[index]
  }

  func reload() {
    categories = MWMBookmarksManager.categoriesFromCatalog()
  }

  func setCategory(visible: Bool, at index: Int) {
    let category = categories[index]
    category.isVisible = visible
    MWMBookmarksManager.setCategory(category.categoryId, isVisible: visible)
  }

  func deleteCategory(at index: Int) {
    MWMBookmarksManager.deleteCategory(category(at: index).categoryId)
    reload()
  }
}
