final class BMCDefaultViewModel: NSObject {
  typealias BM = MWMBookmarksManager

  var view: BMCView!

  private enum Const
  {
    static let minCategoryNameLength: UInt = 0
    static let maxCategoryNameLength: UInt = 60
  }

  private var sections: [BMCSection] = []
  private var permissions: [BMCPermission] = []
  private var categories: [BMCCategory] = []
  private var actions: [BMCAction] = []
  private var notifications: [BMCNotification] = []

  private(set) var isPendingPermission = false
  private var isAuthenticated = false
  private var filesPrepared = false;
  
  private var onPreparedToShareCategory: BMCViewModel.onPreparedToShareHandler?

  var minCategoryNameLength: UInt = Const.minCategoryNameLength
  var maxCategoryNameLength: UInt = Const.maxCategoryNameLength

  override init() {
    super.init()
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
      isPendingPermission = false
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

    if BM.areBookmarksLoaded() {
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

  func areAllCategoriesInvisible() -> Bool {
    var result = true;
    categories.forEach { if !$0.isVisible { result = false } }
    return result
  }

  func updateAllCategoriesVisibility(isShowAll: Bool) {
    categories.forEach {
      $0.isVisible = isShowAll
      BM.setCategory($0.identifier, isVisible: isShowAll)
    }
  }

  func updateCategoryVisibility(category: BMCCategory) {
    category.isVisible = !category.isVisible
    BM.setCategory(category.identifier, isVisible: category.isVisible)
  }

  func addCategory(name: String) {
    guard let section = sections.index(of: .categories) else {
      assertionFailure()
      return
    }
    
    categories.append(BMCCategory(identifier: BM.createCategory(withName: name), title: name))
    view.insert(at: [IndexPath(row: categories.count - 1, section: section)])
  }

  func renameCategory(category: BMCCategory, name: String) {
    category.title = name
    BM.setCategory(category.identifier, name: name)
  }

  func deleteCategory(category: BMCCategory) {
    guard let row = categories.index(of: category), let section = sections.index(of: .categories)
    else {
      assertionFailure()
      return
    }

    categories.remove(at: row)
    BM.deleteCategory(category.identifier)
    view.delete(at: [IndexPath(row: row, section: section)])
  }

  func checkCategory(name: String) -> Bool {
    return BM.checkCategoryName(name)
  }

  func shareCategory(category: BMCCategory, handler: @escaping onPreparedToShareHandler) {
    onPreparedToShareCategory = handler
    BM.shareCategory(category.identifier)
  }

  func finishShareCategory() {
    BM.finishShareCategory()
    onPreparedToShareCategory = nil
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

  func convertAllKMLIfNeeded() {
    let count = BM.filesCountForConversion()
    if count > 0 {
      MWMAlertViewController.activeAlert().presentConvertBookmarksAlert(withCount: count) {
        MWMAlertViewController.activeAlert().presentSpinnerAlert(withTitle: L("converting"),
                                                                 cancel: nil)
        BM.convertAll()
      }
    }
  }

  func addToObserverList() {
    BM.add(self)
  }

  func removeFromObserverList() {
    BM.remove(self)
  }
  
  func setNotificationsEnabled(_ enabled: Bool) {
    BM.setNotificationsEnabled(enabled)
  }
  
  func areNotificationsEnabled() -> Bool {
    return BM.areNotificationsEnabled()
  }

  func requestRestoring() {
    BM.requestRestoring()
  }

  func applyRestoring() {
    BM.applyRestoring()
  }

  func cancelRestoring() {
    if filesPrepared {
      return
    }

    BM.cancelRestoring()
  }
}

extension BMCDefaultViewModel: MWMBookmarksObserver {
  func onRestoringStarted() {
    filesPrepared = false
    MWMAlertViewController.activeAlert().presentSpinnerAlert(withTitle: L("bookmarks_restore_process"))
    { [weak self] in self?.cancelRestoring() }
  }

  func onRestoringFilesPrepared() {
    filesPrepared = true
  }

  func onSynchronizationFinished(_ result: MWMSynchronizationResult) {
    MWMAlertViewController.activeAlert().closeAlert() { [weak self] in
      switch result {
        case .networkError: fallthrough
        case .authError:
          MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_server_title"),
                                                                   message: L("error_server_message"),
                                                                   rightButtonTitle: L("try_again"),
                                                                   leftButtonTitle: L("cancel")) {
            [weak self] in
            self?.requestRestoring()
        }

        case .diskError:
          MWMAlertViewController.activeAlert().presentInternalErrorAlert()

        case .userInterrupted: break
        case .success:
          guard let s = self else { return }
          s.setCategories()
          s.view.update(sections: [.categories])
      }
    }
  }

  func onRestoringRequest(_ result: MWMRestoringRequestResult, deviceName name: String?, backupDate date: Date?) {
    MWMAlertViewController.activeAlert().closeAlert() {
      switch result {
        case .noInternet: MWMAlertViewController.activeAlert().presentNoConnectionAlert()

        case .backupExists:
          guard let date = date else {
            assertionFailure()
            return
          }

          let formatter = DateFormatter()
          formatter.dateStyle = .short
          formatter.timeStyle = .none
          let deviceName = name ?? ""
          let message = String(coreFormat: L("bookmarks_restore_message"),
                              arguments: [formatter.string(from: date), deviceName])

          let cancelAction = { [weak self] in self?.cancelRestoring() } as MWMVoidBlock
          MWMAlertViewController.activeAlert().presentRestoreBookmarkAlert(withMessage: message,
                                                                           rightButtonAction: { [weak self] in
              MWMAlertViewController.activeAlert().presentSpinnerAlert(withTitle: L("bookmarks_restore_process"),
                                                                       cancel: cancelAction)
              self?.applyRestoring()
          }, leftButtonAction: cancelAction)

        case .noBackup:
          MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("bookmarks_restore_empty_title"),
                                                                   message: L("bookmarks_restore_empty_message"),
                                                                   rightButtonTitle: L("ok"),
                                                                   leftButtonTitle: nil,
                                                                   rightButtonAction: nil)

        case .notEnoughDiskSpace: MWMAlertViewController.activeAlert().presentNotEnoughSpaceAlert()

        case .requestError: assertionFailure()
      }
    }
  }

  func onBookmarksLoadFinished() {
    loadData()
    convertAllKMLIfNeeded()
  }

  func onBookmarkDeleted(_: MWMMarkID) {
    loadData()
  }
  
  func onBookmarksCategoryFilePrepared(_ status: MWMBookmarksShareStatus) {
    switch status {
    case .success:
      onPreparedToShareCategory?(.success(BM.shareCategoryURL()))
    case .emptyCategory:
      onPreparedToShareCategory?(.error(title: L("bookmarks_error_title_share_empty"), text: L("bookmarks_error_message_share_empty")))
    case .archiveError: fallthrough
    case .fileError:
      onPreparedToShareCategory?(.error(title: L("dialog_routing_system_error"), text: L("bookmarks_error_message_share_general")))
    }
  }

  func onConversionFinish(_ success: Bool) {
    setCategories()
    view.update(sections: [.categories])
    view.conversionFinished(success: success)
  }
}
