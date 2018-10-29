class DownloadedBookmarksDataSource {
  
  private var categories: [MWMCatalogCategory] = []
  var categoriesCount: NSInteger {
    get {
      return categories.count
    }
  }

  var allCategoriesHidden: Bool {
    get {
      var result = true
      categories.forEach { if $0.visible { result = false } }
      return result
    }
    set {
      categories.forEach {
        $0.visible = !newValue
      }
      MWMBookmarksManager.shared().setCatalogCategoriesVisible(!newValue)
    }
  }

  init() {
    reloadData()
  }

  func category(at index: Int) -> MWMCatalogCategory {
    return categories[index]
  }

  func reloadData() {
    categories = MWMBookmarksManager.shared().categoriesFromCatalog()
  }

  func setCategory(visible: Bool, at index: Int) {
    let category = categories[index]
    category.visible = visible
    MWMBookmarksManager.shared().setCategory(category.categoryId, isVisible: visible)
  }

  func deleteCategory(at index: Int) {
    MWMBookmarksManager.shared().deleteCategory(category(at: index).categoryId)
    reloadData()
  }
}
