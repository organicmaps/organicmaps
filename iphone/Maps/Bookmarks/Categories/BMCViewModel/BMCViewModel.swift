protocol BMCView: AnyObject {
  func update(sections: [BMCSection])
  func delete(at indexPaths: [IndexPath])
  func insert(at indexPaths: [IndexPath])
  func conversionFinished(success: Bool)
}

enum BMCShareCategoryStatus {
  case success(URL)
  case error(title: String, text: String)
}

protocol BMCViewModel: AnyObject {
  var view: BMCView! { get set }
  var isPendingPermission: Bool { get }
  var minCategoryNameLength: UInt { get }
  var maxCategoryNameLength: UInt { get }

  func numberOfSections() -> Int
  func sectionType(section: Int) -> BMCSection
  func sectionIndex(section: BMCSection) -> Int
  func numberOfRows(section: Int) -> Int
  func numberOfRows(section: BMCSection) -> Int

  func item(indexPath: IndexPath) -> BMCModel

  func areAllCategoriesHidden() -> Bool

  func updateAllCategoriesVisibility(isShowAll: Bool)
  func updateCategoryVisibility(category: BMCCategory)

  func addCategory(name: String)
  func renameCategory(category: BMCCategory, name: String)
  func deleteCategory(category: BMCCategory)
  func checkCategory(name: String) -> Bool

  typealias onPreparedToShareHandler = (BMCShareCategoryStatus) -> Void
  func shareCategoryFile(category: BMCCategory, handler: @escaping onPreparedToShareHandler)
  func finishShareCategory()

  func pendingPermission(isPending: Bool)
  func grant(permission: BMCPermission?)

  func convertAllKMLIfNeeded();

  func addToObserverList()
  func removeFromObserverList()

  func setNotificationsEnabled(_ enabled: Bool)
  func areNotificationsEnabled() -> Bool

  func requestRestoring()
  func applyRestoring()
  func cancelRestoring()
  
  func reloadData()
}
