final class BookmarksListViewController: MWMViewController {
  var presenter: IBookmarksListPresenter!

  private var sections: [IBookmarksListSectionViewModel]?
  private let cellStrategy = BookmarksListCellStrategy()

  private var canEdit = false

  @IBOutlet private var tableView: UITableView!
  @IBOutlet private var searchBar: UISearchBar!
  @IBOutlet private var toolBar: UIToolbar!
  @IBOutlet private var sortToolbarItem: UIBarButtonItem!
  @IBOutlet private var moreToolbarItem: UIBarButtonItem!

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

    let toolbarItemAttributes = [NSAttributedString.Key.font: UIFont.medium16(),
                                 NSAttributedString.Key.foregroundColor: UIColor.linkBlue()]

    sortToolbarItem.setTitleTextAttributes(toolbarItemAttributes, for: .normal)
    moreToolbarItem.setTitleTextAttributes(toolbarItemAttributes, for: .normal)
    sortToolbarItem.title = L("sort")
    searchBar.placeholder = L("search_in_the_list")
    cellStrategy.registerCells(tableView)
    cellStrategy.cellCheckHandler = { [weak self] (viewModel, index, checked) in
      self?.presenter.checkItem(in: viewModel, at: index, checked: checked)
    }
    cellStrategy.cellVisibilityHandler = { [weak self] viewModel in
      self?.presenter.toggleVisibility(in: viewModel)
    }
    presenter.viewDidLoad()
    MWMKeyboard.add(self);
  }
  
  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    presenter.viewDidAppear()
  }

  deinit {
    MWMKeyboard.remove(self);
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    updateInfoSize()
  }

  private func updateInfoSize() {
    guard let infoView = infoViewController.view else { return }
    let infoViewSize = infoView.systemLayoutSizeFitting(CGSize(width: view.width, height: 0),
                                                        withHorizontalFittingPriority: .required,
                                                        verticalFittingPriority: .fittingSizeLevel)
    infoView.size = infoViewSize
    tableView.tableHeaderView = infoView
  }

  @IBAction private func onSortItem(_ sender: UIBarButtonItem) {
    presenter.sort()
  }

  @IBAction private func onMoreItem(_ sender: UIBarButtonItem) {
    presenter.more()
  }

  override func setEditing(_ editing: Bool, animated: Bool) {
    super.setEditing(editing, animated: animated)
    tableView.setEditing(editing, animated: animated)
  }
}

extension BookmarksListViewController: UITableViewDataSource {
  func numberOfSections(in tableView: UITableView) -> Int {
    sections?.count ?? 0
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    guard let section = sections?[section] else { fatalError() }
    return section.numberOfItems
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    guard let section = sections?[indexPath.section] else { fatalError() }
    return cellStrategy.tableCell(tableView, for: section, at: indexPath)
  }
}

extension BookmarksListViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    60
  }

  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    guard let section = sections?[section] else { fatalError() }
    return cellStrategy.headerView(tableView, for: section)
  }

  func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    return indexPath
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    guard let section = sections?[indexPath.section] else { fatalError() }
    presenter.selectItem(in: section, at: indexPath.row)
  }

  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    guard let section = sections?[indexPath.section] else { fatalError() }
    return canEdit && section.canEdit
  }

  func tableView(_ tableView: UITableView, willBeginEditingRowAt indexPath: IndexPath) {
    isEditing = true
  }

  func tableView(_ tableView: UITableView, didEndEditingRowAt indexPath: IndexPath?) {
    isEditing = false
  }
  
  func tableView(_ tableView: UITableView,
                 leadingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
    let moveAction = UIContextualAction(style: .normal, title: L("move")) { [weak self] (_, _, completion) in
      guard let section = self?.sections?[indexPath.section] else { fatalError() }
      self?.presenter.moveItem(in: section, at: indexPath.row)
      completion(true)
    }
    return UISwipeActionsConfiguration(actions: [moveAction])
  }

  func tableView(_ tableView: UITableView,
                 trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
    let deleteAction = UIContextualAction(style: .destructive, title: L("delete")) { [weak self] (_, _, completion) in
      guard let section = self?.sections?[indexPath.section] else { fatalError() }
      self?.presenter.deleteItem(in: section, at: indexPath.row)
      completion(true)
    }
    let editAction = UIContextualAction(style: .normal, title: L("edit")) { [weak self] (_, _, completion) in
      guard let section = self?.sections?[indexPath.section] else { fatalError() }
      self?.presenter.editItem(in: section, at: indexPath.row)
      completion(true)
    }
    return UISwipeActionsConfiguration(actions: [deleteAction, editAction])
  }
}

extension BookmarksListViewController: UISearchBarDelegate {
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(true, animated: true)
    navigationController?.setNavigationBarHidden(true, animated: true)
    presenter.activateSearch()
  }

  func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(false, animated: true)
    navigationController?.setNavigationBarHidden(false, animated: true)
    presenter.deactivateSearch()
  }

  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    searchBar.text = nil
    searchBar.resignFirstResponder()
    presenter.cancelSearch()
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    guard !searchText.isEmpty else {
      presenter.cancelSearch()
      return
    }

    presenter.search(searchText)
  }
}

extension BookmarksListViewController: IBookmarksListView {
  func setTitle(_ title: String) {
    self.title = title
  }

  func setInfo(_ info: IBookmakrsListInfoViewModel) {
    infoViewController.info = info
    updateInfoSize()
  }

  func setSections(_ sections: [IBookmarksListSectionViewModel]) {
    self.sections = sections
    tableView.reloadData()
  }

  func setMoreItemTitle(_ itemTitle: String) {
    moreToolbarItem.title = itemTitle
  }

  func showMenu(_ items: [IBookmarksListMenuItem]) {
    let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    items.forEach { item in
      let action = UIAlertAction(title: item.title, style: item.destructive ? .destructive : .default) { _ in
        item.action()
      }
      action.isEnabled = item.enabled
      actionSheet.addAction(action)
    }
    actionSheet.addAction(UIAlertAction(title: L("cancel"), style: .cancel, handler: nil))
    actionSheet.popoverPresentationController?.barButtonItem = sortToolbarItem
    present(actionSheet, animated: true)
  }

  func enableEditing(_ enable: Bool) {
    canEdit = enable
    navigationItem.rightBarButtonItem = enable ? editButtonItem : nil
  }

  func share(_ url: URL, completion: @escaping () -> Void) {
    let shareController = ActivityViewController.share(for: url,
                                                       message: L("share_bookmarks_email_body")) { (_, _, _, _) in
      completion()
    }
    shareController?.present(inParentViewController: self, anchorView: self.toolBar)
  }

  func showError(title: String, message: String) {
    MWMAlertViewController.activeAlert().presentInfoAlert(title, text: message)
  }
}

extension BookmarksListViewController: BookmarksListInfoViewControllerDelegate {
  func didPressDescription() {
    presenter.showDescription()
  }

  func didUpdateContent() {
    updateInfoSize()
  }
}

extension BookmarksListViewController: MWMKeyboardObserver {
  func onKeyboardAnimation() {
    let keyboardHeight = MWMKeyboard.keyboardHeight();
    tableView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: keyboardHeight, right: 0)
  }
}
