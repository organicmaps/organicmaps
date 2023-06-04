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

final class BMCDefaultViewModel: NSObject {
  private let manager = BookmarksManager.shared()

  weak var view: BMCView?

  private var sections: [BMCSection] = []
  private var categories: [BookmarkGroup] = []
  private var actions: [BMCAction] = []
  private var notifications: [BMCNotification] = []

  private(set) var isPendingPermission = false
  private var isAuthenticated = false
  private var filesPrepared = false

  typealias OnPreparedToShareHandler = (BMCShareCategoryStatus) -> Void
  private var onPreparedToShareCategory: OnPreparedToShareHandler?

  let minCategoryNameLength: UInt = 0
  let maxCategoryNameLength: UInt = 60

  override init() {
    super.init()
    reloadData()
  }

  private func setCategories() {
    categories = manager.userCategories()
  }

  private func setActions() {
    actions = [.create]
  }

  private func setNotifications() {
    notifications = [.load]
  }

  func reloadData() {
    sections = []

    if manager.areBookmarksLoaded() {
      sections.append(.categories)
      setCategories()

      sections.append(.actions)
      setActions()
    } else {
      sections.append(.notifications)
      setNotifications()
    }
    view?.update(sections: [])
  }
}

extension BMCDefaultViewModel {
  func numberOfSections() -> Int {
    return sections.count
  }

  func sectionType(section: Int) -> BMCSection {
    return sections[section]
  }

  func sectionIndex(section: BMCSection) -> Int {
    return sections.firstIndex(of: section)!
  }

  func numberOfRows(section: Int) -> Int {
    return numberOfRows(section: sectionType(section: section))
  }

  func numberOfRows(section: BMCSection) -> Int {
    switch section {
    case .categories: return categories.count
    case .actions: return actions.count
    case .notifications: return notifications.count
    }
  }

  func category(at index: Int) -> BookmarkGroup {
    return categories[index]
  }

  func action(at index: Int) -> BMCAction {
    return actions[index]
  }

  func notification(at index: Int) -> BMCNotification {
    return notifications[index]
  }

  func areAllCategoriesHidden() -> Bool {
    var result = true
    categories.forEach { if $0.isVisible { result = false } }
    return result
  }

  func updateAllCategoriesVisibility(isShowAll: Bool) {
    manager.setUserCategoriesVisible(isShowAll)
  }

  func addCategory(name: String) {
    guard let section = sections.firstIndex(of: .categories) else {
      assertionFailure()
      return
    }

    categories.append(manager.category(withId: manager.createCategory(withName: name)))
    view?.insert(at: [IndexPath(row: categories.count - 1, section: section)])
  }

  func deleteCategory(at index: Int) {
    guard let section = sections.firstIndex(of: .categories) else {
      assertionFailure()
      return
    }

    let category = categories[index]
    categories.remove(at: index)
    manager.deleteCategory(category.categoryId)
    view?.delete(at: [IndexPath(row: index, section: section)])
  }

  func checkCategory(name: String) -> Bool {
    return manager.checkCategoryName(name)
  }

  func shareCategoryFile(at index: Int, handler: @escaping OnPreparedToShareHandler) {
    let category = categories[index]
    onPreparedToShareCategory = handler
    manager.shareCategory(category.categoryId)
  }

  func finishShareCategory() {
    manager.finishShareCategory()
    onPreparedToShareCategory = nil
  }

  func addToObserverList() {
    manager.add(self)
  }

  func removeFromObserverList() {
    manager.remove(self)
  }

  func setNotificationsEnabled(_ enabled: Bool) {
    manager.setNotificationsEnabled(enabled)
  }

  func areNotificationsEnabled() -> Bool {
    return manager.areNotificationsEnabled()
  }
}

extension BMCDefaultViewModel: BookmarksObserver {

  func onBookmarksLoadFinished() {
    reloadData()
  }

  func onBookmarkDeleted(_: MWMMarkID) {
    reloadData()
  }

  func onBookmarksCategoryFilePrepared(_ status: BookmarksShareStatus) {
    switch status {
    case .success:
      onPreparedToShareCategory?(.success(manager.shareCategoryURL()))
    case .emptyCategory:
      onPreparedToShareCategory?(.error(title: L("bookmarks_error_title_share_empty"), text: L("bookmarks_error_message_share_empty")))
    case .archiveError: fallthrough
    case .fileError:
      onPreparedToShareCategory?(.error(title: L("dialog_routing_system_error"), text: L("bookmarks_error_message_share_general")))
    }
  }
}
