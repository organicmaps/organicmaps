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
  private var permissions: [BMCPermission] = []
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

  private func setPermissions() {
    isAuthenticated = User.isAuthenticated()
    if !isAuthenticated {
      Statistics.logEvent(kStatBookmarksSyncProposalShown, withParameters: [kStatHasAuthorization: 0])
      permissions = [.signup]
    } else if !manager.isCloudEnabled() {
      Statistics.logEvent(kStatBookmarksSyncProposalShown, withParameters: [kStatHasAuthorization: 1])
      isPendingPermission = false
      permissions = [.backup]
    } else {
      isPendingPermission = false
      permissions = [.restore(manager.lastSynchronizationDate())]
    }
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

    sections.append(.permissions)
    setPermissions()

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
    case .permissions: return permissions.count
    case .categories: return categories.count
    case .actions: return actions.count
    case .notifications: return notifications.count
    }
  }

  func permission(at index: Int) -> BMCPermission {
    return permissions[index]
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

  func pendingPermission(isPending: Bool) {
    isPendingPermission = isPending
    setPermissions()
    view?.update(sections: [.permissions])
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
                              kStatHasAuthorization: isAuthenticated ? 1 : 0
                            ])
        manager.setCloudEnabled(true)
      case .restore:
        assertionFailure("Not implemented")
      }
    }
    pendingPermission(isPending: false)
  }

  func convertAllKMLIfNeeded() {
    let count = manager.filesCountForConversion()
    if count > 0 {
      MWMAlertViewController.activeAlert().presentConvertBookmarksAlert(withCount: count) { [weak self] in
        MWMAlertViewController.activeAlert().presentSpinnerAlert(withTitle: L("converting"),
                                                                 cancel: nil)
        self?.manager.convertAll()
      }
    }
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

  func requestRestoring() {
    let statusStr: String
    switch NetworkPolicy.shared().connectionType {
    case .none:
      statusStr = kStatOffline
    case .wifi:
      statusStr = kStatWifi
    case .cellular:
      statusStr = kStatMobile
    }
    Statistics.logEvent(kStatBookmarksRestoreProposalClick, withParameters: [kStatNetwork: statusStr])
    manager.requestRestoring()
  }

  func applyRestoring() {
    manager.applyRestoring()
    Statistics.logEvent(kStatBookmarksRestoreProposalSuccess)
  }

  func cancelRestoring() {
    if filesPrepared {
      return
    }

    manager.cancelRestoring()
    Statistics.logEvent(kStatBookmarksRestoreProposalCancel)
  }

  func showSyncErrorMessage() {
    MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_server_title"),
                                                             message: L("error_server_message"),
                                                             rightButtonTitle: L("try_again"),
                                                             leftButtonTitle: L("cancel")) {
                                                              [weak self] in
                                                              self?.requestRestoring()
    }
  }
}

extension BMCDefaultViewModel: BookmarksObserver {
  func onBackupStarted() {
    Statistics.logEvent(kStatBookmarksSyncStarted)
  }

  func onRestoringStarted() {
    filesPrepared = false
    MWMAlertViewController.activeAlert().presentSpinnerAlert(withTitle: L("bookmarks_restore_process")) { [weak self] in self?.cancelRestoring() }
  }

  func onRestoringFilesPrepared() {
    filesPrepared = true
  }

  func onSynchronizationFinished(_ result: MWMSynchronizationResult) {
    MWMAlertViewController.activeAlert().closeAlert { [weak self] in
      guard let s = self else { return }
      switch result {
      case .invalidCall:
        s.showSyncErrorMessage()
        Statistics.logEvent(kStatBookmarksSyncError, withParameters: [kStatType: kStatInvalidCall])
      case .networkError:
        s.showSyncErrorMessage()
        Statistics.logEvent(kStatBookmarksSyncError, withParameters: [kStatType: kStatNetwork])
      case .authError:
        s.showSyncErrorMessage()
        Statistics.logEvent(kStatBookmarksSyncError, withParameters: [kStatType: kStatAuth])
      case .diskError:
        MWMAlertViewController.activeAlert().presentInternalErrorAlert()
        Statistics.logEvent(kStatBookmarksSyncError, withParameters: [kStatType: kStatDisk])
      case .userInterrupted:
        Statistics.logEvent(kStatBookmarksSyncError, withParameters: [kStatType: kStatUserInterrupted])
      case .success:
        s.reloadData()
        Statistics.logEvent(kStatBookmarksSyncSuccess)
      @unknown default:
        fatalError()
      }
    }
  }

  func onRestoringRequest(_ result: MWMRestoringRequestResult, deviceName name: String?, backupDate date: Date?) {
    MWMAlertViewController.activeAlert().closeAlert {
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
        Statistics.logEvent(kStatBookmarksRestoreProposalError,
                            withParameters: [kStatType: kStatNoBackup, kStatError: ""])
        MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("bookmarks_restore_empty_title"),
                                                                 message: L("bookmarks_restore_empty_message"),
                                                                 rightButtonTitle: L("ok"),
                                                                 leftButtonTitle: nil,
                                                                 rightButtonAction: nil)

      case .notEnoughDiskSpace:
        Statistics.logEvent(kStatBookmarksRestoreProposalError,
                            withParameters: [kStatType: kStatDisk, kStatError: "Not enough disk space"])
        MWMAlertViewController.activeAlert().presentNotEnoughSpaceAlert()
      case .requestError:
        assertionFailure()
      }
    }
  }

  func onBookmarksLoadFinished() {
    reloadData()
    convertAllKMLIfNeeded()
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

  func onConversionFinish(_ success: Bool) {
    setCategories()
    view?.update(sections: [.categories])
    view?.conversionFinished(success: success)
  }
}
