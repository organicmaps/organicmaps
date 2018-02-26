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

  private func updateCategoryName(category: BMCCategory?) {
    let isNewCategory = (category == nil)
    let alert = UIAlertController(title: L(isNewCategory ? "bookmarks_create_new_group" : "rename"), message: nil, preferredStyle: .alert)

    alert.addTextField { textField in
      textField.placeholder = L("bookmark_set_name")
      if let category = category {
        textField.text = category.title
      }
    }

    alert.addAction(UIAlertAction(title: L("cancel").capitalized, style: .cancel, handler: nil))
    alert.addAction(UIAlertAction(title: L(isNewCategory ? "create" : "ok").capitalized, style: .default) { [weak alert, viewModel] _ in
      guard let categoryName = alert?.textFields?.first?.text, !categoryName.isEmpty,
        let viewModel = viewModel else { return }
      if let category = category {
        viewModel.renameCategory(category: category, name: categoryName)
      } else {
        viewModel.addCategory(name: categoryName)
      }
    })

    present(alert, animated: true, completion: nil)
  }

  private func shareCategory(category: BMCCategory, anchor: UIView) {
    let shareOnSuccess = { [viewModel] (url: URL) in
      typealias AVC = MWMActivityViewController
      let fileName = (url.lastPathComponent as NSString).deletingPathExtension
      let message = String(coreFormat: L("share_bookmarks_email_body"), arguments: [fileName])
      let shareController = AVC.share(for: url, message: message) { [viewModel] _, _, _, _ in
        viewModel?.endShareCategory(category: category)
      }
      shareController!.present(inParentViewController: self, anchorView: anchor)
    }

    let showAlertOnError = { (title: String, text: String) in
      MWMAlertViewController.activeAlert().presentInfoAlert(title, text: text)
    }

    let shareCategoryStatus = viewModel.beginShareCategory(category: category)
    switch shareCategoryStatus {
    case let .success(url): shareOnSuccess(url)
    case let .error(title, text): showAlertOnError(title, text)
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
                                               permittedArrowDirections: .any,
                                               sourceComponent: .bookmarks,
                                               successHandler: { _ in onComplete(true) },
                                               errorHandler: { _ in onComplete(false) },
                                               completionHandler: {
                                                 $0.dismiss(animated: true, completion: nil)
      })
      present(authVC, animated: true, completion: nil)
    }
  }

  private func editCategory(category: BMCCategory, anchor: UIView) {
    let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)

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
      categoriesHeader.isShowAll = !viewModel.areAllCategoriesVisible()
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
    categoriesHeader.isShowAll = !viewModel.areAllCategoriesVisible()
  }

  func moreAction(category: BMCCategory, anchor: UIView) {
    editCategory(category: category, anchor: anchor)
  }
}

extension BMCViewController: BMCPermissionsHeaderDelegate {
  func collapseAction(isCollapsed: Bool) {
    permissionsHeader.isCollapsed = !isCollapsed
    update(sections: [.permissions])
  }
}

extension BMCViewController: BMCCategoriesHeaderDelegate {
  func visibilityAction(isShowAll: Bool) {
    viewModel.updateAllCategoriesVisibility(isShowAll: isShowAll)
    categoriesHeader.isShowAll = !viewModel.areAllCategoriesVisible()
  }
}
