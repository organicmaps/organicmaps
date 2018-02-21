final class BMCDefaultViewModel: NSObject {
  var view: BMCView!

  private var sections: [BMCSection] = []
  private var permissions: [BMCPermission] = []
  private var categories: [BMCCategory] = []
  private var actions: [BMCAction] = []
  private var notifications: [BMCNotification] = []

  private(set) var isPendingPermission = false
  private var isAuthenticated = false

  typealias BM = MWMBookmarksManager

  override init() {
    super.init()
    MWMBookmarksManager.add(self)
    loadData()
  }

  private func setPermissions() {
    isAuthenticated = MWMAuthorizationViewModel.isAuthenticated()
    if !isAuthenticated {
      Statistics.logEvent(kStatBookmarksSyncProposalShown, withParameters: [kStatHasAuthorization: 0])
      permissions = [.signup]
    } else if !BM.isCloudEnabled() {
      Statistics.logEvent(kStatBookmarksSyncProposalShown, withParameters: [kStatHasAuthorization: 1])
      isPendingPermission = false
      permissions = [.backup]
    } else {
      isPendingPermission = true
      permissions = [.restore(BM.lastSynchronizationDate())]
    }
  }

  private func setCategories() {
    categories = BM.groupsIdList().map { categoryId -> BMCCategory in
      let title = BM.getCategoryName(categoryId)!
      let count = BM.getCategoryMarksCount(categoryId) + BM.getCategoryTracksCount(categoryId)
      let isVisible = BM.isCategoryVisible(categoryId)
      return BMCCategory(identifier: categoryId, title: title, count: count, isVisible: isVisible)
    }
  }

  private func setActions() {
    actions = [.create]
  }

  private func setNotifications() {
    notifications.append(.load)
  }

  private func loadData() {
    sections = []

    sections.append(.permissions)
    setPermissions()

    if MWMBookmarksManager.areBookmarksLoaded() {
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

extension BMCDefaultViewModel: BMCViewModel {
  func numberOfSections() -> Int {
    return sections.count
  }

  func sectionType(section: Int) -> BMCSection {
    return sections[section]
  }

  func sectionIndex(section: BMCSection) -> Int {
    return sections.index(of: section)!
  }

  func numberOfRows(section: Int) -> Int {
    return numberOfRows(section: sectionType(section: section))
  }

  func numberOfRows(section: BMCSection) -> Int {
    switch section {
    case .permissions: return permissions.count
    case .categories: return categories.count
    case .actions: return actions.count
    case .notifications: return notifications.count
    }
  }

  func item(indexPath: IndexPath) -> BMCModel {
    let (section, row) = (indexPath.section, indexPath.row)
    switch sectionType(section: section) {
    case .permissions: return permissions[row]
    case .categories: return categories[row]
    case .actions: return actions[row]
    case .notifications: return notifications[row]
    }
  }

  func areAllCategoriesVisible() -> Bool {
    return categories.reduce(true) { $0 && $1.isVisible }
  }

  func updateAllCategoriesVisibility(isShowAll: Bool) {
    categories.forEach { $0.isVisible = isShowAll }
    BM.setAllCategoriesVisible(isShowAll)
  }

  func updateCategoryVisibility(category: BMCCategory) {
    category.isVisible = !category.isVisible
    BM.setCategory(category.identifier, isVisible: category.isVisible)
  }

  func addCategory(name: String) {
    categories.append(BMCCategory(identifier: BM.createCategory(withName: name), title: name))
    view.update(sections: [.categories])
  }

  func renameCategory(category: BMCCategory, name: String) {
    category.title = name
    BM.setCategory(category.identifier, name: name)
  }

  func deleteCategory(category: BMCCategory) {
    categories.remove(at: categories.index(of: category)!)
    BM.deleteCategory(category.identifier)
    view.update(sections: [.categories])
  }

  func beginShareCategory(category: BMCCategory) -> URL {
    return BM.beginShareCategory(category.identifier)
  }

  func endShareCategory(category: BMCCategory) {
    BM.endShareCategory(category.identifier)
  }

  func pendingPermission(isPending: Bool) {
    isPendingPermission = isPending
    setPermissions()
    view.update(sections: [.permissions])
  }

  func grant(permission: BMCPermission?) {
    if let permission = permission {
      switch permission {
      case .signup:
        assertionFailure()
      case .backup:
        Statistics.logEvent(kStatBookmarksSyncProposalApproved,
                            withParameters: [
                              kStatNetwork: Statistics.connectionTypeString(),
                              kStatHasAuthorization: isAuthenticated ? 1 : 0,
        ])
        BM.setCloudEnabled(true)
      case .restore:
        assertionFailure("Not implemented")
      }
    }
    pendingPermission(isPending: false)
  }
}

extension BMCDefaultViewModel: MWMBookmarksObserver {
  func onBookmarksLoadFinished() {
    loadData()
  }
}
