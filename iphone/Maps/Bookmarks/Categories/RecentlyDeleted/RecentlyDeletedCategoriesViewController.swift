final class RecentlyDeletedCategoriesViewController: MWMTableViewController {

  private enum LocalizedStrings {
    static let edit = L("edit")
    static let done = L("done")
    static let delete = L("delete")
    static let deleteAll = L("delete_all")
    static let recover = L("recover")
    static let recoverAll = L("recover_all")
    static let recentlyDeleted = L("bookmarks_recently_deleted")
    static let searchInTheList = L("search_in_the_list")
  }

  private lazy var editButton = UIBarButtonItem(title: LocalizedStrings.edit, style: .done, target: self, action: #selector(editButtonDidTap))
  private lazy var recoverButton = UIBarButtonItem(title: LocalizedStrings.recover, style: .done, target: self, action: #selector(recoverButtonDidTap))
  private lazy var deleteButton = UIBarButtonItem(title: LocalizedStrings.delete, style: .done, target: self, action: #selector(deleteButtonDidTap))
  private let searchController = UISearchController(searchResultsController: nil)
  private let viewModel: RecentlyDeletedCategoriesViewModel

  init(viewModel: RecentlyDeletedCategoriesViewModel = RecentlyDeletedCategoriesViewModel()) {
    self.viewModel = viewModel
    super.init(nibName: nil, bundle: nil)

    viewModel.didReceiveEvent = { [weak self] event in
      self?.processEvent(event)
    }
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
    viewModel.handleAction(.fetchRecentlyDeletedCategories)
  }

  private func setupView() {
    setupNavigationBar()
    setupToolBar()
    setupSearchBar()
    setupTableView()
  }

  private func setupNavigationBar() {
    title = LocalizedStrings.recentlyDeleted
    navigationItem.rightBarButtonItem = editButton
  }

  private func setupToolBar() {
    let flexibleSpace = UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: nil, action: nil)
    toolbarItems = [flexibleSpace, recoverButton, flexibleSpace, deleteButton, flexibleSpace]
    navigationController?.isToolbarHidden = true
  }

  private func setupSearchBar() {
    searchController.searchBar.placeholder = LocalizedStrings.searchInTheList
    searchController.obscuresBackgroundDuringPresentation = false
    searchController.hidesNavigationBarDuringPresentation = alternativeSizeClass(iPhone: true, iPad: false)
    searchController.searchBar.delegate = self
    searchController.searchBar.applyTheme()
    navigationItem.searchController = searchController
    navigationItem.hidesSearchBarWhenScrolling = false
  }

  private func setupTableView() {
    tableView.allowsMultipleSelectionDuringEditing = true
    tableView.register(cell: RecentlyDeletedTableViewCell.self)
  }

  private func processEvent(_ event: RecentlyDeletedCategoriesViewModel.Event) {
    switch event {
    case .updateCategories(let categories):
      updateDataSource(categories)
    case .removeCategory(let indexPaths):
      tableView.deleteRows(at: indexPaths, with: .automatic)
    case .updateInteractionMode(let interactionMode):
      updateInteractionMode(interactionMode)
    }
  }

  private func updateDataSource(_ dataSource: [RecentlyDeletedCategoriesViewModel.State.SectionModel]) {
    if dataSource.isEmpty {
      self.tableView.reloadData()
    } else if self.tableView.numberOfSections > 0 {
      let indexes = IndexSet(integersIn: 0...dataSource.count - 1)
      tableView.update { self.tableView.reloadSections(indexes, with: .none) }
    }
  }

  private func updateInteractionMode(_ interactionMode: RecentlyDeletedCategoriesViewModel.State.InteractionMode) {
    switch interactionMode {
    case .normal:
      tableView.setEditing(false, animated: true)
      navigationController?.setToolbarHidden(true, animated: true)
      editButton.title = LocalizedStrings.edit
      searchController.searchBar.isUserInteractionEnabled = true
    case .searching:
      tableView.setEditing(false, animated: true)
      navigationController?.setToolbarHidden(true, animated: true)
      editButton.title = LocalizedStrings.edit
      searchController.searchBar.isUserInteractionEnabled = true
    case .editingAndNothingSelected:
      tableView.setEditing(true, animated: true)
      navigationController?.setToolbarHidden(false, animated: true)
      editButton.title = LocalizedStrings.done
      recoverButton.title = LocalizedStrings.recoverAll
      deleteButton.title = LocalizedStrings.deleteAll
      searchController.searchBar.isUserInteractionEnabled = false
    case .editingAndSomeSelected:
      recoverButton.title = LocalizedStrings.recover
      deleteButton.title = LocalizedStrings.delete
      searchController.searchBar.isUserInteractionEnabled = false
    }
  }

  // MARK: - Actions
  @objc private func editButtonDidTap() {
    tableView.setEditing(!tableView.isEditing, animated: true)
    viewModel.handleAction(tableView.isEditing ? .startSelecting : .cancelSelecting)
  }

  @objc private func recoverButtonDidTap() {
    viewModel.handleAction(.recoverSelected)
  }

  @objc private func deleteButtonDidTap() {
    viewModel.handleAction(.deleteSelected)
  }

  // MARK: - UITableViewDataSource & UITableViewDelegate
  override func numberOfSections(in tableView: UITableView) -> Int {
    viewModel.state.filteredDataSource.count
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    viewModel.state.filteredDataSource[section].categories.count
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: RecentlyDeletedTableViewCell.self, indexPath: indexPath)
    let category = viewModel.state.filteredDataSource[indexPath.section].categories[indexPath.row]
    cell.configureWith(category)
    return cell
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard tableView.isEditing else {
      tableView.deselectRow(at: indexPath, animated: true)
      return
    }
    viewModel.handleAction(.select(at: indexPath))
  }

  override func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
    guard tableView.isEditing else { return }
    guard let selectedIndexPaths = tableView.indexPathsForSelectedRows, !selectedIndexPaths.isEmpty else {
      viewModel.handleAction(.deselectAll)
      return
    }
    viewModel.handleAction(.deselect(at: indexPath))
  }

  override func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
    let deleteAction = UIContextualAction(style: .destructive, title: LocalizedStrings.delete) { [weak self] (_, _, completion) in
      self?.viewModel.handleAction(.delete(at: indexPath))
      completion(true)
    }
    let recoverAction = UIContextualAction(style: .normal, title: LocalizedStrings.recover) { [weak self] (_, _, completion) in
      self?.viewModel.handleAction(.recover(at: indexPath))
      completion(true)
    }
    return UISwipeActionsConfiguration(actions: [deleteAction, recoverAction])
  }
}

// MARK: - UISearchBarDelegate
extension RecentlyDeletedCategoriesViewController: UISearchBarDelegate {
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(true, animated: true)
    viewModel.handleAction(.startSearching)
  }

  func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(false, animated: true)
  }

  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    searchBar.text = nil
    searchBar.resignFirstResponder()
    viewModel.handleAction(.cancelSearching)
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    viewModel.handleAction(.search(searchText))
  }
}
