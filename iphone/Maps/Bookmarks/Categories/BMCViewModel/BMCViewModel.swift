protocol BMCView: AnyObject {
  func update(sections: [BMCSection])
}

enum BMCShareCategoryStatus {
  case success(URL)
  case error(title: String, text: String)
}

protocol BMCViewModel: AnyObject {
  var view: BMCView! { get set }
  var isPendingPermission: Bool { get }

  func numberOfSections() -> Int
  func sectionType(section: Int) -> BMCSection
  func sectionIndex(section: BMCSection) -> Int
  func numberOfRows(section: Int) -> Int
  func numberOfRows(section: BMCSection) -> Int

  func item(indexPath: IndexPath) -> BMCModel

  func areAllCategoriesVisible() -> Bool
  func updateAllCategoriesVisibility(isShowAll: Bool)
  func updateCategoryVisibility(category: BMCCategory)

  func addCategory(name: String)
  func renameCategory(category: BMCCategory, name: String)
  func deleteCategory(category: BMCCategory)

  func beginShareCategory(category: BMCCategory) -> BMCShareCategoryStatus
  func endShareCategory(category: BMCCategory)

  func pendingPermission(isPending: Bool)
  func grant(permission: BMCPermission?)
}
