final class BMCViewController: MWMViewController {
  private var viewModel: BMCViewModel! {
    didSet {
      viewModel.view = self
      tableView.dataSource = self
    }
  }

  @IBOutlet private weak var tableView: UITableView! {
    didSet {
      let cells = [
        BMCPermissionsCell.self,
        BMCPermissionsPendingCell.self,
        BMCCategoryCell.self,
        BMCActionsCreateCell.self,
        BMCNotificationsCell.self,
      ]
      tableView.registerNibs(cells)
      tableView.registerNibForHeaderFooterView(BMCCategoriesHeader.self)
    }
  }

  @IBOutlet private var permissionsHeader: BMCPermissionsHeader! {
    didSet {
      permissionsHeader.delegate = self
    }
  }

  @IBOutlet private var actionsHeader: UIView!
  @IBOutlet private var notificationsHeader: BMCNotificationsHeader!

  override func viewDidLoad() {
    super.viewDidLoad()
    viewModel = BMCDefaultViewModel()
    tableView.separatorColor = UIColor.blackDividers()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    viewModel.reloadData()
  }
  
  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    // Disable all notifications in BM on appearance of this view.
    // It allows to significantly improve performance in case of bookmarks
    // modification. All notifications will be sent on controller's disappearance.
    viewModel.setNotificationsEnabled(false)
    viewModel.addToObserverList()
    viewModel.convertAllKMLIfNeeded()
  }
  
  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    // Allow to send all notifications in BM.
    viewModel.setNotificationsEnabled(true)
    viewModel.removeFromObserverList()
  }

  private func createNewCategory() {
    alertController.presentCreateBookmarkCategoryAlert(withMaxCharacterNum: viewModel.maxCategoryNameLength,
                                                       minCharacterNum: viewModel.minCategoryNameLength)
    { [weak viewModel] (name: String!) -> Bool in
      guard let model = viewModel else { return false }
      if model.checkCategory(name: name) {
        model.addCategory(name: name)
        return true
      }

      return false
    }
  }

  private func shareCategoryFile(category: BMCCategory, anchor: UIView) {
    viewModel.shareCategoryFile(category: category) {
      switch $0 {
      case let .success(url):
        let shareController = MWMActivityViewController.share(for: url,
                                                              message: L("share_bookmarks_email_body"))
        { [weak self] _, _, _, _ in
          self?.viewModel?.finishShareCategory()
        }
        shareController?.present(inParentViewController: self, anchorView: anchor)
      case let .error(title, text):
        MWMAlertViewController.activeAlert().presentInfoAlert(title, text: text)
      }
    }
  }


  private func shareCategory(category: BMCCategory, anchor: UIView) {
    let storyboard = UIStoryboard.instance(.sharing)
    let shareController = storyboard.instantiateInitialViewController() as! BookmarksSharingViewController
    shareController.categoryId = category.identifier
    
    MapViewController.topViewController().navigationController?.pushViewController(shareController,
                                                                                   animated: true)
  }
  
  private func openCategorySettings(category: BMCCategory) {
    let storyboard = UIStoryboard.instance(.categorySettings)
    let settingsController = storyboard.instantiateInitialViewController() as! CategorySettingsViewController
    settingsController.categoryId = category.identifier
    settingsController.maxCategoryNameLength = viewModel.maxCategoryNameLength
    settingsController.minCategoryNameLength = viewModel.minCategoryNameLength
    settingsController.delegate = self
    
    MapViewController.topViewController().navigationController?.pushViewController(settingsController,
                                                                                   animated: true)
  }

  private func openCategory(category: BMCCategory) {
    let bmViewController = BookmarksVC(category: category.identifier)!
    bmViewController.delegate = self
    MapViewController.topViewController().navigationController?.pushViewController(bmViewController,
                                                                                   animated: true)
  }

  private func editCategory(category: BMCCategory, anchor: UIView) {
    let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    if let ppc = actionSheet.popoverPresentationController {
      ppc.sourceView = anchor
      ppc.sourceRect = anchor.bounds
    }

    let settings = L("list_settings")
    actionSheet.addAction(UIAlertAction(title: settings, style: .default, handler: { _ in
      self.openCategorySettings(category: category)
      Statistics.logEvent(kStatBookmarksListSettingsClick,
                          withParameters: [kStatOption : kStatListSettings])
    }))
    let showHide = L(category.isVisible ? "hide_from_map" : "zoom_to_country")
    actionSheet.addAction(UIAlertAction(title: showHide, style: .default, handler: { _ in
      self.visibilityAction(category: category)
      Statistics.logEvent(kStatBookmarksListSettingsClick,
                          withParameters: [kStatOption : kStatMakeInvisibleOnMap])
    }))
    let exportFile = L("export_file")
    actionSheet.addAction(UIAlertAction(title: exportFile, style: .default, handler: { _ in
      self.shareCategoryFile(category: category, anchor: anchor)
      Statistics.logEvent(kStatBookmarksListSettingsClick,
                          withParameters: [kStatOption : kStatSendAsFile])
    }))
    let share = L("sharing_options")
    let shareAction = UIAlertAction(title: share, style: .default, handler: { _ in
      self.shareCategory(category: category, anchor: anchor)
      Statistics.logEvent(kStatBookmarksListSettingsClick,
                          withParameters: [kStatOption : kStatSharingOptions])
    })
    shareAction.isEnabled = MWMBookmarksManager.shared().isCategoryNotEmpty(category.identifier)
    actionSheet.addAction(shareAction)
    let delete = L("delete_list")
    let deleteAction = UIAlertAction(title: delete, style: .destructive, handler: { [viewModel] _ in
      viewModel!.deleteCategory(category: category)
      Statistics.logEvent(kStatBookmarksListSettingsClick,
                          withParameters: [kStatOption : kStatDeleteGroup])
    })
    deleteAction.isEnabled = (viewModel.numberOfRows(section: .categories) > 1)
    actionSheet.addAction(deleteAction)
    let cancel = L("cancel")
    actionSheet.addAction(UIAlertAction(title: cancel, style: .cancel, handler: nil))

    present(actionSheet, animated: true, completion: nil)
  }
}

extension BMCViewController: BMCView {
  func update(sections: [BMCSection]) {
    if sections.isEmpty {
      tableView.reloadData()
    } else {
      let indexes = IndexSet(sections.map { viewModel.sectionIndex(section: $0) })
      tableView.update { tableView.reloadSections(indexes, with: .automatic) }
    }
  }

  func insert(at indexPaths: [IndexPath]) {
    tableView.insertRows(at: indexPaths, with: .automatic)
  }

  func delete(at indexPaths: [IndexPath]) {
    tableView.deleteRows(at: indexPaths, with: .automatic)
  }

  func conversionFinished(success: Bool) {
    MWMAlertViewController.activeAlert().closeAlert {
      if !success {
        MWMAlertViewController.activeAlert().presentBookmarkConversionErrorAlert()
      }
    }
  }
}

extension BMCViewController: UITableViewDataSource {
  func numberOfSections(in _: UITableView) -> Int {
    return viewModel.numberOfSections()
  }

  func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch viewModel.sectionType(section: section) {
    case .permissions:
      return permissionsHeader.isCollapsed ? 0 : viewModel.numberOfRows(section: section)
    case .categories: fallthrough
    case .actions: fallthrough
    case .notifications: return viewModel.numberOfRows(section: section)
    }
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    func dequeCell<Cell>(_ cell: Cell.Type) -> Cell where Cell: UITableViewCell {
      return tableView.dequeueReusableCell(cell: cell, indexPath: indexPath)
    }
    switch viewModel.item(indexPath: indexPath) {
    case let permission as BMCPermission:
      if viewModel.isPendingPermission {
        return dequeCell(BMCPermissionsPendingCell.self).config(permission: permission)
      } else {
        return dequeCell(BMCPermissionsCell.self).config(permission: permission, delegate: self)
      }
    case let category as BMCCategory:
      return dequeCell(BMCCategoryCell.self).config(category: category, delegate: self)
    case let model as BMCAction:
      return dequeCell(BMCActionsCreateCell.self).config(model: model)
    case is BMCNotification:
      return dequeCell(BMCNotificationsCell.self)
    default:
      assertionFailure()
      return UITableViewCell()
    }
  }
}

extension BMCViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    if viewModel.sectionType(section: indexPath.section) != .categories {
      return false
    }

    return viewModel.numberOfRows(section: .categories) > 1
  }

  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCellEditingStyle, forRowAt indexPath: IndexPath) {
    guard let item = viewModel.item(indexPath: indexPath) as? BMCCategory,
    editingStyle == .delete,
    viewModel.sectionType(section: indexPath.section) == .categories
    else {
      assertionFailure()
      return
    }

    viewModel.deleteCategory(category: item)
  }

  func tableView(_: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    switch viewModel.sectionType(section: section) {
    case .permissions: fallthrough
    case .notifications: fallthrough
    case .categories: return 48
    case .actions: return 24
    }
  }

  func tableView(_: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    switch viewModel.sectionType(section: section) {
    case .permissions: return permissionsHeader
    case .categories:
      let categoriesHeader = tableView.dequeueReusableHeaderFooterView(BMCCategoriesHeader.self)
      categoriesHeader.isShowAll = viewModel.areAllCategoriesHidden()
      categoriesHeader.title = L("bookmarks_groups")
      categoriesHeader.delegate = self
      return categoriesHeader
    case .actions: return actionsHeader
    case .notifications: return notificationsHeader
    }
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    switch viewModel.item(indexPath: indexPath) {
    case is BMCPermission:
      return
    case let category as BMCCategory:
      openCategory(category: category)
    case let action as BMCAction:
      switch action {
      case .create: createNewCategory()
      }
    default:
      assertionFailure()
    }
  }
}

extension BMCViewController: BMCPermissionsCellDelegate {
  func permissionAction(permission: BMCPermission, anchor: UIView) {
    switch permission {
    case .signup:
      viewModel.pendingPermission(isPending: true)
      signup(anchor: anchor, onComplete: { [viewModel] success in
        viewModel!.grant(permission: success ? .backup : nil)
      })
    case .backup:
      viewModel.grant(permission: permission)
    case .restore:
      viewModel.requestRestoring()
    }
  }
}

extension BMCViewController: BMCCategoryCellDelegate {
  func visibilityAction(category: BMCCategory) {
    viewModel.updateCategoryVisibility(category: category)
    if let categoriesHeader = tableView.headerView(forSection: viewModel.sectionIndex(section: .categories)) as? BMCCategoriesHeader {
      categoriesHeader.isShowAll = viewModel.areAllCategoriesHidden()
    }
  }

  func moreAction(category: BMCCategory, anchor: UIView) {
    editCategory(category: category, anchor: anchor)
  }
}

extension BMCViewController: BMCPermissionsHeaderDelegate {
  func collapseAction(isCollapsed: Bool) {
    permissionsHeader.isCollapsed = !isCollapsed
    let sectionIndex = viewModel.sectionIndex(section: .permissions)
    let rowsInSection = viewModel.numberOfRows(section: .permissions)
    var rowIndexes = [IndexPath]()
    for rowIndex in 0..<rowsInSection {
      rowIndexes.append(IndexPath(row: rowIndex, section: sectionIndex))
    }
    if (permissionsHeader.isCollapsed) {
      delete(at: rowIndexes)
    } else {
      insert(at: rowIndexes)
    }
  }
}

extension BMCViewController: BMCCategoriesHeaderDelegate {
  func visibilityAction(_ categoriesHeader: BMCCategoriesHeader) {
    viewModel.updateAllCategoriesVisibility(isShowAll: categoriesHeader.isShowAll)
    categoriesHeader.isShowAll = viewModel.areAllCategoriesHidden()
  }
}

extension BMCViewController: CategorySettingsViewControllerDelegate {
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didEndEditing categoryId: MWMMarkGroupID) {
    navigationController?.popViewController(animated: true)
  }
  
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didDelete categoryId: MWMMarkGroupID) {
    navigationController?.popViewController(animated: true)
  }
}

extension BMCViewController: BookmarksVCDelegate {
  func bookmarksVCdidUpdateCategory(_ viewController: BookmarksVC!) {
    // for now we did necessary interface update in -viewWillAppear
  }

  func bookmarksVCdidDeleteCategory(_ viewController: BookmarksVC!) {
    navigationController?.popViewController(animated: true)
  }
}
