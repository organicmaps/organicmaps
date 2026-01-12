final class BMCViewController: MWMViewController {
  private var viewModel: BMCDefaultViewModel! {
    didSet {
      viewModel.view = self
      tableView.dataSource = self
    }
  }

  private weak var coordinator: BookmarksCoordinator?

  @IBOutlet private weak var tableView: UITableView! {
    didSet {
      let cells = [
        BMCCategoryCell.self,
        BMCActionsCell.self,
        BMCNotificationsCell.self,
      ]
      tableView.registerNibs(cells)
      tableView.registerNibForHeaderFooterView(BMCCategoriesHeader.self)
    }
  }

  @IBOutlet private var actionsHeader: UIView!
  @IBOutlet private var notificationsHeader: BMCNotificationsHeader!

  init(coordinator: BookmarksCoordinator?) {
    super.init(nibName: nil, bundle: nil)
    self.coordinator = coordinator
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    view.setStyle(.pressBackground)
    viewModel = BMCDefaultViewModel()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    viewModel.addToObserverList()
    viewModel.reloadData()
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
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

  private func shareCategoryFile(at index: Int, fileType: KmlFileType, anchor: UIView) {
    UIApplication.shared.showLoadingOverlay()
    viewModel.shareCategoryFile(at: index, fileType: fileType, handler: sharingResultHandler(anchorView: anchor))
  }

  private func shareAllCategories(anchor: UIView?) {
    UIApplication.shared.showLoadingOverlay()
    viewModel.shareAllCategories(handler: sharingResultHandler(anchorView: anchor))
  }

  private func sharingResultHandler(anchorView: UIView?) -> SharingResultCompletionHandler {
    { [weak self] status, url in
      UIApplication.shared.hideLoadingOverlay {
        guard let self else { return }
        switch status {
        case .success:
          let shareController = ActivityViewController.share(for: url, message: L("share_bookmarks_email_body"))
          { [weak self] _, _, _, _ in
            self?.viewModel?.finishShareCategory()
          }
          shareController.present(inParentViewController: self, anchorView: anchorView)
        case .emptyCategory:
          MWMAlertViewController.activeAlert().presentInfoAlert(L("bookmarks_error_title_share_empty"),
                                                                text: L("bookmarks_error_message_share_empty"))
        case .fileError, .archiveError:
          MWMAlertViewController.activeAlert().presentInfoAlert(L("dialog_routing_system_error"),
                                                                text: L("bookmarks_error_message_share_general"))
        }
      }
    }
  }

  private func showImportDialog() {
    DocumentPicker.shared.present(from: self) { [viewModel] urls in
      viewModel?.importCategories(from: urls)
    }
  }

  private func openCategorySettings(category: BookmarkGroup) {
    let settingsController = CategorySettingsViewController(bookmarkGroup: BookmarksManager.shared().category(withId: category.categoryId))
    settingsController.delegate = self

    MapViewController.shared()?.navigationController?.pushViewController(settingsController, animated: true)
  }

  private func openCategory(category: BookmarkGroup) {
    let bmViewController = BookmarksListBuilder.build(markGroupId: category.categoryId,
                                                      bookmarksCoordinator: coordinator,
                                                      delegate: self)
    MapViewController.shared()?.navigationController?.pushViewController(bmViewController, animated: true)
  }

  private func setCategoryVisible(_ visible: Bool, at index: Int) {
    let category = viewModel.category(at: index)
    BookmarksManager.shared().setCategory(category.categoryId, isVisible: visible)
    if let categoriesHeader = tableView.headerView(forSection: viewModel.sectionIndex(section: .categories)) as? BMCCategoriesHeader {
      categoriesHeader.isShowAll = viewModel.areAllCategoriesHidden()
    }
  }

  private func editCategory(at index: Int, anchor: UIView) {
    let category = viewModel.category(at: index)
    let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    if let ppc = actionSheet.popoverPresentationController {
      ppc.sourceView = anchor
      ppc.sourceRect = anchor.bounds
    }

    let settings = L("edit")
    actionSheet.addAction(UIAlertAction(title: settings, style: .default, handler: { _ in
      self.openCategorySettings(category: category)
    }))
    let showHide = L(category.isVisible ? "hide_from_map" : "zoom_to_country")
    actionSheet.addAction(UIAlertAction(title: showHide, style: .default, handler: { _ in
      self.setCategoryVisible(!category.isVisible, at: index)
      let sectionIndex = self.viewModel.sectionIndex(section: .categories)
      self.tableView.reloadRows(at: [IndexPath(row: index, section: sectionIndex)], with: .none)
    }))
    actionSheet.addAction(UIAlertAction(title: L("export_file"), style: .default, handler: { _ in
      self.shareCategoryFile(at: index, fileType: .text, anchor: anchor)
    }))
    actionSheet.addAction(UIAlertAction(title: L("export_file_gpx"), style: .default, handler: { _ in
      self.shareCategoryFile(at: index, fileType: .gpx, anchor: anchor)
    }))
    let delete = L("delete_list")
    let deleteAction = UIAlertAction(title: delete, style: .destructive, handler: { [viewModel] _ in
      viewModel!.deleteCategory(at: index)
    })
    deleteAction.isEnabled = (viewModel.canDeleteCategory())
    actionSheet.addAction(deleteAction)
    let cancel = L("cancel")
    actionSheet.addAction(UIAlertAction(title: cancel, style: .cancel, handler: nil))

    present(actionSheet, animated: true, completion: nil)
  }

  private func openRecentlyDeleted() {
    let recentlyDeletedController = RecentlyDeletedCategoriesViewController(viewModel: RecentlyDeletedCategoriesViewModel(bookmarksManager: BookmarksManager.shared()))
    MapViewController.shared()?.navigationController?.pushViewController(recentlyDeletedController, animated: true)
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
    case .categories: fallthrough
    case .actions, .recentlyDeleted: fallthrough
    case .notifications: return viewModel.numberOfRows(section: section)
    }
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    func dequeCell<Cell>(_ cell: Cell.Type) -> Cell where Cell: UITableViewCell {
      return tableView.dequeueReusableCell(cell: cell, indexPath: indexPath)
    }

    switch viewModel.sectionType(section: indexPath.section) {
    case .categories:
      return dequeCell(BMCCategoryCell.self).config(category: viewModel.category(at: indexPath.row),
                                                    delegate: self)
    case .actions:
      return dequeCell(BMCActionsCell.self).config(model: viewModel.action(at: indexPath.row))
    case .recentlyDeleted:
      return dequeCell(BMCActionsCell.self).config(model: viewModel.recentlyDeletedCategories())
    case .notifications:
      return dequeCell(BMCNotificationsCell.self)
    }
  }
}

extension BMCViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    if viewModel.sectionType(section: indexPath.section) != .categories {
      return false
    }

    return viewModel.canDeleteCategory()
  }

  func tableView(_ tableView: UITableView,
                 commit editingStyle: UITableViewCell.EditingStyle,
                 forRowAt indexPath: IndexPath) {
    guard editingStyle == .delete,
      viewModel.sectionType(section: indexPath.section) == .categories else {
        assertionFailure()
        return
    }

    viewModel.deleteCategory(at: indexPath.row)
  }

  func tableView(_: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    switch viewModel.sectionType(section: section) {
    case .notifications: fallthrough
    case .categories: return 48
    case .actions, .recentlyDeleted: return 24
    }
  }

  func tableView(_: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    switch viewModel.sectionType(section: section) {
    case .categories:
      let categoriesHeader = tableView.dequeueReusableHeaderFooterView(BMCCategoriesHeader.self)
      categoriesHeader.isShowAll = viewModel.areAllCategoriesHidden()
      categoriesHeader.title = L("bookmark_lists")
      categoriesHeader.delegate = self
      return categoriesHeader
    case .actions, .recentlyDeleted: return actionsHeader
    case .notifications: return notificationsHeader
    }
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    switch viewModel.sectionType(section: indexPath.section) {
    case .categories:
      openCategory(category: viewModel.category(at: indexPath.row))
    case .actions:
      switch viewModel.action(at: indexPath.row) {
      case .create: createNewCategory()
      case .exportAll: shareAllCategories(anchor: tableView.cellForRow(at: indexPath))
      case .import: showImportDialog()
      default:
        assertionFailure()
      }
    case .recentlyDeleted: openRecentlyDeleted()
    default:
      assertionFailure()
    }
  }
}

extension BMCViewController: BMCCategoryCellDelegate {
  func cell(_ cell: BMCCategoryCell, didCheck visible: Bool) {
    guard let indexPath = tableView.indexPath(for: cell) else {
      assertionFailure()
      return
    }

    setCategoryVisible(visible, at: indexPath.row)
  }

  func cell(_ cell: BMCCategoryCell, didPress moreButton: UIButton) {
    guard let indexPath = tableView.indexPath(for: cell) else {
      assertionFailure()
      return
    }

    editCategory(at: indexPath.row, anchor: moreButton)
  }
}

extension BMCViewController: BMCCategoriesHeaderDelegate {
  func visibilityAction(_ categoriesHeader: BMCCategoriesHeader) {
    viewModel.updateAllCategoriesVisibility(isShowAll: categoriesHeader.isShowAll)
    categoriesHeader.isShowAll = viewModel.areAllCategoriesHidden()
    tableView.reloadData()
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

extension BMCViewController: BookmarksListDelegate {
  func bookmarksListDidDeleteGroup() {
    navigationController?.popViewController(animated: true)
  }
}
