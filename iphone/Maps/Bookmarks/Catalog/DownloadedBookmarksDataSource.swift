class DownloadedBookmarksDataSource {
  
  private var categories: [MWMCategory] = []
  var categoriesCount: NSInteger {
    get {
      return categories.count
    }
  }

  var allCategoriesHidden: Bool {
    get {
      var result = true
      categories.forEach { if $0.isVisible { result = false } }
      return result
    }
    set {
      MWMBookmarksManager.shared().setCatalogCategoriesVisible(!newValue)
    }
  }
  
  var guideIds: String {
    get {
      return MWMBookmarksManager.shared().getGuidesIds()
    }
  }

  init() {
    reloadData()
  }

  func category(at index: Int) -> MWMCategory {
    return categories[index]
  }

  func reloadData() {
    categories = MWMBookmarksManager.shared().categoriesFromCatalog()
  }

  func setCategory(visible: Bool, at index: Int) {
    let category = categories[index]
    category.isVisible = visible
  }

  func deleteCategory(at index: Int) {
    MWMBookmarksManager.shared().deleteCategory(category(at: index).categoryId)
    reloadData()
  }
  
  func getServerId(at index: Int) -> String {
    return MWMBookmarksManager.shared().getServerId(category(at: index).categoryId)
  }
  
  func isGuide(at index: Int) -> Bool {
    return MWMBookmarksManager.shared().isGuide(category(at: index).categoryId)
  }
}
