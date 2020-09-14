class DownloadedBookmarksDataSource {
  
  private var categories: [BookmarkGroup] = []
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
      BookmarksManager.shared().setCatalogCategoriesVisible(!newValue)
    }
  }
  
  var guideIds: String {
    get {
      return BookmarksManager.shared().getGuidesIds()
    }
  }

  init() {
    reloadData()
  }

  func category(at index: Int) -> BookmarkGroup {
    return categories[index]
  }

  func reloadData() {
    categories = BookmarksManager.shared().categoriesFromCatalog()
  }

  func setCategory(visible: Bool, at index: Int) {
    let category = categories[index]
    BookmarksManager.shared().setCategory(category.categoryId, isVisible: visible)
  }

  func deleteCategory(at index: Int) {
    BookmarksManager.shared().deleteCategory(category(at: index).categoryId)
    reloadData()
  }
  
  func getServerId(at index: Int) -> String {
    return BookmarksManager.shared().getServerId(category(at: index).categoryId)
  }
  
  func isGuide(at index: Int) -> Bool {
    return BookmarksManager.shared().isGuide(category(at: index).categoryId)
  }
}
