final class BMCViewController: MWMViewController {
  private var viewModel: BMCViewModel! {
    didSet {
      viewModel.view = self
      tableView.dataSource = self
    }
  }

  @IBOutlet private weak var tableView: UITableView! {
    didSet {
      tableView.registerNibs(cells: [
        BMCPermissionsCell.self,
        BMCPermissionsPendingCell.self,
        BMCCategoryCell.self,
        BMCActionsCreateCell.self,
        BMCNotificationsCell.self,
      ])
    }
  }

  @IBOutlet private var permissionsHeader: BMCPermissionsHeader! {
    didSet {
      permissionsHeader.isCollapsed = false
      permissionsHeader.delegate = self
    }
  }

  @IBOutlet private var categoriesHeader: BMCCategoriesHeader! {
    didSet {
      categoriesHeader.delegate = self
    }
  }

  @IBOutlet private var actionsHeader: UIView!
  @IBOutlet private var notificationsHeader: BMCNotificationsHeader!

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("bookmarks")
    viewModel = BMCDefaultViewModel()
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

  private func updateCategoryName(category: BMCCategory?) {
    let isNewCategory = (category == nil)
    alertController.presentCreateBookmarkCategoryAlert(withMaxCharacterNum: viewModel.maxCategoryNameLength,
                                                       minCharacterNum: viewModel.minCategoryNameLength,
                                                       isNewCategory: isNewCategory)
    { [weak viewModel] (name: String!) -> Bool in
      guard let model = viewModel else { return false }
      if model.checkCategory(name: name) {
        if isNewCategory {
          model.addCategory(name: name)
        } else {
          model.renameCategory(category: category!, name: name)
        }

        return true
      }

      return false
    }
  }

  private func shareCategory(category: BMCCategory, anchor: UIView) {
    let shareOnSuccess = { [viewModel] (url: URL) in
      typealias AVC = MWMActivityViewController
      let message = L("share_bookmarks_email_body")
      let shareController = AVC.share(for: url, message: message) { [viewModel] _, _, _, _ in
        viewModel?.finishShareCategory()
      }
      shareController!.present(inParentViewController: self, anchorView: anchor)
    }

    let showAlertOnError = { (title: String, text: String) in
      MWMAlertViewController.activeAlert().presentInfoAlert(title, text: text)
    }
    
    viewModel.shareCategory(category: category) { (status: BMCShareCategoryStatus) in
      switch status {
      case let .success(url): shareOnSuccess(url)
      case let .error(title, text): showAlertOnError(title, text)
      }
    }
  }

  private func openCategory(category: BMCCategory) {
    let bmViewController = BookmarksVC(category: category.identifier)!
    navigationController!.pushViewController(bmViewController, animated: true)
  }

  private func signup(anchor: UIView, onComplete: @escaping (Bool) -> Void) {
    if MWMAuthorizationViewModel.hasSocialToken() {
      MWMAuthorizationViewModel.checkAuthentication(with: .bookmarks, onComplete: onComplete)
    } else {
      let authVC = AuthorizationViewController(popoverSourceView: anchor,
                                               sourceComponent: .bookmarks,
                                               permittedArrowDirections: .any,
                                               successHandler: { _ in onComplete(true) },
                                               errorHandler: { _ in onComplete(false) })
      present(authVC, animated: true, completion: nil)
    }
  }

  private func editCategory(category: BMCCategory, anchor: UIView) {
    let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    if let ppc = actionSheet.popoverPresentationController {
      ppc.sourceView = anchor
      ppc.sourceRect = anchor.bounds
    }

    let rename = L("rename").capitalized
    actionSheet.addAction(UIAlertAction(title: rename, style: .default, handler: { _ in
      self.updateCategoryName(category: category)
    }))
    let showHide = L(category.isVisible ? "hide" : "show").capitalized
    actionSheet.addAction(UIAlertAction(title: showHide, style: .default, handler: { _ in
      self.visibilityAction(category: category)
    }))
    let share = L("share").capitalized
    actionSheet.addAction(UIAlertAction(title: share, style: .default, handler: { _ in
      self.shareCategory(category: category, anchor: anchor)
    }))
    let delete = L("delete").capitalized
    let deleteAction = UIAlertAction(title: delete, style: .destructive, handler: { [viewModel] _ in
      viewModel!.deleteCategory(category: category)
    })
    deleteAction.isEnabled = (viewModel.numberOfRows(section: .categories) > 1)
    actionSheet.addAction(deleteAction)
    let cancel = L("cancel").capitalized
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
      categoriesHeader.isShowAll = viewModel.areAllCategoriesInvisible()
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
      case .create: updateCategoryName(category: nil)
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
    case .restore: assertionFailure()
    }
  }
}

extension BMCViewController: BMCCategoryCellDelegate {
  func visibilityAction(category: BMCCategory) {
    viewModel.updateCategoryVisibility(category: category)
    categoriesHeader.isShowAll = viewModel.areAllCategoriesInvisible()
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
  func visibilityAction(isShowAll: Bool) {
    viewModel.updateAllCategoriesVisibility(isShowAll: isShowAll)
    categoriesHeader.isShowAll = viewModel.areAllCategoriesInvisible()
  }
}
