final class BookmarksListViewController: MWMViewController {
  var presenter: IBookmarksListPresenter!

  private enum Constants {
    static let headerHeight = CGFloat(60)
  }

  private var sections: [IBookmarksListSectionViewModel]?
  private let cellStrategy = BookmarksListCellStrategy()

  private var canEdit = false
  private var isSearchActive = false
  private var defaultToolbarItems: [UIBarButtonItem] = []
  private var selectedItemIds = Set<BookmarksListItemId>()

  @IBOutlet private var tableView: UITableView!
  @IBOutlet private var toolBar: UIToolbar!
  @IBOutlet private var sortToolbarItem: UIBarButtonItem!
  @IBOutlet private var moreToolbarItem: UIBarButtonItem!
  private let searchController = UISearchController(searchResultsController: nil)
  private lazy var selectBarButtonItem = UIBarButtonItem(title: L("select"),
                                                         style: .plain,
                                                         target: self,
                                                         action: #selector(selectButtonDidTap))
  private lazy var selectAllBarButtonItem = UIBarButtonItem(title: L("select_all"),
                                                            style: .plain,
                                                            target: self,
                                                            action: #selector(selectAllButtonDidTap))
  private lazy var cancelBarButtonItem = UIBarButtonItem(title: L("cancel"),
                                                         style: .plain,
                                                         target: self,
                                                         action: #selector(cancelButtonDidTap))
  private lazy var moveToolbarItem: UIBarButtonItem = {
    let item = UIBarButtonItem(image: UIImage(named: "ic_folder")?.withRenderingMode(.alwaysTemplate),
                               style: .plain,
                               target: self,
                               action: #selector(moveButtonDidTap))
    item.accessibilityLabel = L("move")
    item.tintColor = .linkBlue
    return item
  }()

  private lazy var colorToolbarItem: UIBarButtonItem = {
    let item = UIBarButtonItem(image: UIImage(named: "ic_palette")?.withRenderingMode(.alwaysTemplate),
                               style: .plain,
                               target: self,
                               action: #selector(colorButtonDidTap))
    item.accessibilityLabel = L("change_color")
    item.tintColor = .linkBlue
    return item
  }()

  private lazy var deleteToolbarItem: UIBarButtonItem = {
    let item = UIBarButtonItem(image: UIImage(named: "ic_route_manager_trash")?.withRenderingMode(.alwaysTemplate),
                               style: .plain,
                               target: self,
                               action: #selector(deleteButtonDidTap))
    item.accessibilityLabel = L("delete")
    item.tintColor = .redPrimary
    return item
  }()

  private lazy var infoViewController: BookmarksListInfoViewController = {
    let infoViewController = BookmarksListInfoViewController()
    infoViewController.delegate = self
    addChild(infoViewController)
    tableView.tableHeaderView = infoViewController.view
    infoViewController.didMove(toParent: self)
    return infoViewController
  }()

  override func viewDidLoad() {
    super.viewDidLoad()

    let toolbarItemAttributes = [NSAttributedString.Key.font: UIFont.medium16.dynamic,
                                 NSAttributedString.Key.foregroundColor: UIColor.linkBlue]
    sortToolbarItem.setTitleTextAttributes(toolbarItemAttributes, for: .normal)
    moreToolbarItem.setTitleTextAttributes(toolbarItemAttributes, for: .normal)
    sortToolbarItem.title = L("sort")
    defaultToolbarItems = toolBar.items ?? []

    extendedLayoutIncludesOpaqueBars = true
    searchController.searchBar.placeholder = L("search_in_the_list")
    searchController.obscuresBackgroundDuringPresentation = false
    searchController.hidesNavigationBarDuringPresentation = alternativeSizeClass(iPhone: true, iPad: false)
    searchController.searchBar.delegate = self
    searchController.searchBar.applyTheme()
    navigationItem.searchController = searchController
    navigationItem.hidesSearchBarWhenScrolling = false

    tableView.allowsMultipleSelectionDuringEditing = true
    cellStrategy.registerCells(tableView)
    presenter.viewDidLoad()
    MWMKeyboard.add(self)
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    presenter.viewDidAppear()
  }

  deinit {
    MWMKeyboard.remove(self)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    updateInfoSize()
    tableView.contentInset.bottom = toolBar.height
  }

  private func updateInfoSize() {
    guard let infoView = infoViewController.view else { return }
    let infoViewSize = infoView.systemLayoutSizeFitting(CGSize(width: view.width, height: 0),
                                                        withHorizontalFittingPriority: .required,
                                                        verticalFittingPriority: .fittingSizeLevel)
    infoView.size = infoViewSize
    tableView.tableHeaderView = infoView
  }

  @IBAction private func onSortItem(_: UIBarButtonItem) {
    presenter.sort()
  }

  @IBAction private func onMoreItem(_: UIBarButtonItem) {
    presenter.more()
  }

  @objc private func selectButtonDidTap() {
    // An open swipe action leaves the table in editing mode, which would make entering multi-select a no-op.
    tableView.setEditing(false, animated: false)
    setEditing(true, animated: true)
  }

  @objc private func cancelButtonDidTap() {
    setEditing(false, animated: true)
  }

  @objc private func selectAllButtonDidTap() {
    guard isEditing, let sections else { return }

    if areAllEditableItemsSelected {
      clearSelection(animated: false)
      updateSelectionActionsState()
      return
    }

    for (sectionIndex, section) in sections.enumerated() {
      for (row, item) in section.editableItems.enumerated() {
        selectedItemIds.insert(item.itemId)
        tableView.selectRow(at: IndexPath(row: row, section: sectionIndex), animated: false, scrollPosition: .none)
      }
    }
    updateSelectionActionsState()
  }

  @objc private func moveButtonDidTap() {
    guard !selectedItemIds.isEmpty else { return }
    presenter.moveItems(with: selectedItemIds)
  }

  @objc private func colorButtonDidTap() {
    guard !selectedItemIds.isEmpty else { return }
    presenter.changeColor(of: selectedItemIds)
  }

  @objc private func deleteButtonDidTap() {
    guard !selectedItemIds.isEmpty else { return }

    let itemIds = selectedItemIds
    setEditing(false, animated: true)
    presenter.deleteItems(with: itemIds)
  }

  override func setEditing(_ editing: Bool, animated: Bool) {
    super.setEditing(editing, animated: animated)
    tableView.setEditing(editing, animated: animated)
    updateNavigationButton()
    searchController.searchBar.searchTextField.isEnabled = !editing
    searchController.searchBar.isUserInteractionEnabled = !editing
    updateToolbar(editing: editing, animated: animated)

    if !editing {
      clearSelection(animated: animated)
    }
  }

  private func updateToolbar(editing: Bool, animated: Bool) {
    guard editing else {
      toolBar.setItems(defaultToolbarItems, animated: animated)
      return
    }

    updateSelectionActionsState()
    toolBar.setItems([sortToolbarItem,
                      UIBarButtonItem(systemItem: .flexibleSpace),
                      moveToolbarItem,
                      colorToolbarItem,
                      deleteToolbarItem],
                     animated: animated)
  }

  private func updateSelectionActionsState() {
    let isEnabled = !selectedItemIds.isEmpty
    moveToolbarItem.isEnabled = isEnabled
    colorToolbarItem.isEnabled = isEnabled
    deleteToolbarItem.isEnabled = isEnabled
    selectAllBarButtonItem.title = L(areAllEditableItemsSelected ? "deselect_all" : "select_all")
    selectAllBarButtonItem.isEnabled = editableItemsCount > 0
  }

  private var editableItemsCount: Int {
    sections?.reduce(0) { $0 + $1.editableItems.count } ?? 0
  }

  private var areAllEditableItemsSelected: Bool {
    editableItemsCount > 0 && selectedItemIds.count == editableItemsCount
  }

  private func updateNavigationButton() {
    guard canEdit else {
      navigationItem.leftBarButtonItem = nil
      navigationItem.rightBarButtonItem = nil
      return
    }

    if isEditing {
      navigationItem.leftBarButtonItem = selectAllBarButtonItem
      navigationItem.rightBarButtonItem = cancelBarButtonItem
    } else {
      navigationItem.leftBarButtonItem = nil
      selectBarButtonItem.isEnabled = !isSearchActive
      navigationItem.rightBarButtonItem = selectBarButtonItem
    }
  }

  private func itemId(at indexPath: IndexPath) -> BookmarksListItemId? {
    guard let section = sections?[indexPath.section] else { fatalError() }
    let items = section.editableItems
    guard items.indices.contains(indexPath.row) else { return nil }
    return items[indexPath.row].itemId
  }

  private func restoreSelection() {
    guard isEditing, let sections else { return }

    // Drop ids whose items disappeared after reload, then re-apply the surviving selection.
    var restoredItemIds = Set<BookmarksListItemId>()
    for (sectionIndex, section) in sections.enumerated() {
      for (row, item) in section.editableItems.enumerated() where selectedItemIds.contains(item.itemId) {
        restoredItemIds.insert(item.itemId)
        tableView.selectRow(at: IndexPath(row: row, section: sectionIndex), animated: false, scrollPosition: .none)
      }
    }
    selectedItemIds = restoredItemIds
    updateSelectionActionsState()
  }

  private func clearSelection(animated: Bool) {
    selectedItemIds.removeAll()
    tableView.indexPathsForSelectedRows?.forEach { tableView.deselectRow(at: $0, animated: animated) }
  }
}

extension BookmarksListViewController: UITableViewDataSource {
  func numberOfSections(in _: UITableView) -> Int {
    sections?.count ?? 0
  }

  func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
    guard let section = sections?[section] else { fatalError() }
    return section.numberOfItems
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let section = sections?[indexPath.section] else { fatalError() }
    return cellStrategy.tableCell(tableView, for: section, at: indexPath)
  }
}

extension BookmarksListViewController: UITableViewDelegate {
  func tableView(_: UITableView, heightForHeaderInSection _: Int) -> CGFloat {
    Constants.headerHeight
  }

  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    guard let section = sections?[section] else { fatalError() }
    return cellStrategy.headerView(tableView, for: section)
  }

  func tableView(_: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    if isEditing {
      guard let section = sections?[indexPath.section] else { fatalError() }
      return section.canEdit ? indexPath : nil
    }
    return indexPath
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard !isEditing else {
      if let itemId = itemId(at: indexPath) {
        selectedItemIds.insert(itemId)
      }
      updateSelectionActionsState()
      return
    }

    tableView.deselectRow(at: indexPath, animated: true)
    guard let section = sections?[indexPath.section] else { fatalError() }
    presenter.selectItem(in: section, at: indexPath.row)
  }

  func tableView(_: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    guard let section = sections?[indexPath.section] else { fatalError() }
    return canEdit && section.canEdit
  }

  func tableView(_: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
    guard let section = sections?[indexPath.section] else { fatalError() }
    return section.canEdit
  }

  func tableView(_: UITableView, didDeselectRowAt indexPath: IndexPath) {
    guard isEditing else { return }
    if let itemId = itemId(at: indexPath) {
      selectedItemIds.remove(itemId)
    }
    updateSelectionActionsState()
  }

  func tableView(_: UITableView,
                 leadingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
    let moveAction = UIContextualAction(style: .normal, title: L("move")) { [weak self] _, _, completion in
      guard let self, let itemId = self.itemId(at: indexPath) else {
        completion(false)
        return
      }
      presenter.moveItems(with: [itemId])
      completion(true)
    }
    return UISwipeActionsConfiguration(actions: [moveAction])
  }

  func tableView(_: UITableView,
                 trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
    let deleteAction = UIContextualAction(style: .destructive, title: L("delete")) { [weak self] _, _, completion in
      guard let self, let itemId = self.itemId(at: indexPath) else {
        completion(false)
        return
      }
      self.presenter.deleteItems(with: [itemId])
      completion(true)
    }
    let editAction = UIContextualAction(style: .normal, title: L("edit")) { [weak self] _, _, completion in
      guard let section = self?.sections?[indexPath.section] else { fatalError() }
      self?.presenter.editItem(in: section, at: indexPath.row)
      completion(true)
    }
    return UISwipeActionsConfiguration(actions: [deleteAction, editAction])
  }

  func tableView(_: UITableView, accessoryButtonTappedForRowWith indexPath: IndexPath) {
    guard let section = sections?[indexPath.section] else { fatalError() }
    presenter.editItem(in: section, at: indexPath.row)
  }
}

extension BookmarksListViewController: UISearchBarDelegate {
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    isSearchActive = true
    updateNavigationButton()
    toolBar.setHidden(true)
    searchBar.setShowsCancelButton(true, animated: true)
    presenter.activateSearch()
  }

  func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
    isSearchActive = !(searchBar.text?.isEmpty ?? true)
    updateNavigationButton()
    toolBar.setHidden(false)
    searchBar.setShowsCancelButton(false, animated: true)
    presenter.deactivateSearch()
  }

  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    searchBar.text = nil
    isSearchActive = false
    updateNavigationButton()
    searchBar.resignFirstResponder()
    presenter.cancelSearch()
  }

  func searchBar(_: UISearchBar, textDidChange searchText: String) {
    guard !searchText.isEmpty else {
      presenter.cancelSearch()
      return
    }

    presenter.search(searchText)
  }
}

extension BookmarksListViewController: IBookmarksListView {
  func setInfo(_ info: IBookmarksListInfoViewModel) {
    navigationItem.backButtonTitle = info.title
    infoViewController.info = info
    updateInfoSize()
  }

  func setSections(_ sections: [IBookmarksListSectionViewModel]) {
    self.sections = sections
    tableView.reloadData()
    restoreSelection()
  }

  func showMenu(_ items: [IBookmarksListMenuItem], from source: BookmarkToolbarButtonSource) {
    let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    for item in items {
      let action = UIAlertAction(title: item.title, style: item.destructive ? .destructive : .default) { _ in
        item.action()
      }
      action.isEnabled = item.enabled
      actionSheet.addAction(action)
    }
    actionSheet.addAction(UIAlertAction(title: L("cancel"), style: .cancel, handler: nil))
    let barButtonItem = switch source {
    case .sort: sortToolbarItem
    case .more: moreToolbarItem
    }
    actionSheet.popoverPresentationController?.barButtonItem = barButtonItem
    present(actionSheet, animated: true)
  }

  func showColorPicker(anchor: UIView?, currentColor: UIColor?, _ completionHandler: ((UIColor) -> Void)?) {
    ColorPicker.shared.present(from: self, anchor: anchor, currentColor: currentColor, completionHandler: completionHandler)
  }

  func showBatchColorPicker(_ completionHandler: ((UIColor) -> Void)?) {
    ColorPicker.shared.present(from: self,
                               anchor: colorToolbarItem,
                               currentColor: nil,
                               completionHandler: completionHandler)
  }

  func finishEditing() {
    // Move and color finish while their modal controller is still being dismissed.
    // Avoid running a competing toolbar transition underneath that dismissal.
    setEditing(false, animated: false)
  }

  func enableEditing(_ enable: Bool) {
    canEdit = enable
    if !enable, isEditing {
      setEditing(false, animated: false)
    }
    updateNavigationButton()
  }

  func share(_ url: URL, displayName: String, completion: @escaping () -> Void) {
    let shareController = ActivityViewController.share(for: url,
                                                       message: L("share_bookmarks_email_body"),
                                                       displayName: displayName) { _, _, _, _ in
      completion()
    }
    shareController.present(inParentViewController: self, anchorView: toolBar)
  }

  func showError(title: String, message: String) {
    MWMAlertViewController.activeAlert().presentInfoAlert(title, text: message)
  }
}

extension BookmarksListViewController: BookmarksListInfoViewControllerDelegate {
  func didPressDescription() {
    presenter.showDescription()
  }

  func didPressEdit() {
    presenter.editCategory()
  }

  func didUpdateContent() {
    updateInfoSize()
  }
}

extension BookmarksListViewController: MWMKeyboardObserver {
  func onKeyboardAnimation() {
    let keyboardHeight = MWMKeyboard.keyboardHeight()
    tableView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: keyboardHeight, right: 0)
  }
}
