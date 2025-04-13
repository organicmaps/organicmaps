final class RecentlyDeletedCategoriesViewController: MWMViewController {

  private enum LocalizedStrings {
    static let clear = L("clear")
    static let delete = L("delete")
    static let deleteAll = L("delete_all")
    static let recover = L("recover")
    static let recoverAll = L("recover_all")
    static let recentlyDeleted = L("bookmarks_recently_deleted")
    static let searchInTheList = L("search_in_the_list")
  }

  private let tableView = UITableView(frame: .zero, style: .plain)

  private lazy var clearButton = UIBarButtonItem(title: LocalizedStrings.clear, style: .done, target: self, action: #selector(clearButtonDidTap))
  private lazy var recoverButton = UIBarButtonItem(title: LocalizedStrings.recover, style: .done, target: self, action: #selector(recoverButtonDidTap))
  private lazy var deleteButton = UIBarButtonItem(title: LocalizedStrings.delete, style: .done, target: self, action: #selector(deleteButtonDidTap))
  private let searchController = UISearchController(searchResultsController: nil)
  private let viewModel: RecentlyDeletedCategoriesViewModel

  init(viewModel: RecentlyDeletedCategoriesViewModel = RecentlyDeletedCategoriesViewModel(bookmarksManager: BookmarksManager.shared())) {
    self.viewModel = viewModel
    super.init(nibName: nil, bundle: nil)

    viewModel.stateDidChange = { [weak self] state in
      self?.updateState(state)
    }
    viewModel.filteredDataSourceDidChange = { [weak self] dataSource in
      guard let self else { return }
      if dataSource.isEmpty {
        self.tableView.reloadData()
      } else {
        let indexes = IndexSet(integersIn: 0...dataSource.count - 1)
        self.tableView.update { self.tableView.reloadSections(indexes, with: .automatic) }
      }
    }
    viewModel.onCategoriesIsEmpty = { [weak self] in
      self?.goBack()
    }
  }
  
  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    navigationController?.setToolbarHidden(true, animated: true)
  }

  private func setupView() {
    extendedLayoutIncludesOpaqueBars = true
    setupNavigationBar()
    setupToolBar()
    setupSearchBar()
    setupTableView()
    layout()
    updateState(viewModel.state)
  }

  private func setupNavigationBar() {
    title = LocalizedStrings.recentlyDeleted
  }

  private func setupToolBar() {
    let flexibleSpace = UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: nil, action: nil)
    toolbarItems = [flexibleSpace, recoverButton, flexibleSpace, deleteButton, flexibleSpace]
    navigationController?.isToolbarHidden = false
  }

  private func setupSearchBar() {
    searchController.searchBar.placeholder = LocalizedStrings.searchInTheList
    searchController.obscuresBackgroundDuringPresentation = false
    searchController.hidesNavigationBarDuringPresentation = false
    searchController.searchBar.delegate = self
    searchController.searchBar.applyTheme()
    navigationItem.searchController = searchController
    navigationItem.hidesSearchBarWhenScrolling = true
  }

  private func setupTableView() {
    tableView.setStyles([.tableView, .pressBackground])
    tableView.allowsMultipleSelectionDuringEditing = true
    tableView.register(cell: RecentlyDeletedTableViewCell.self)
    tableView.setEditing(true, animated: false)
    tableView.translatesAutoresizingMaskIntoConstraints = false
    tableView.dataSource = self
    tableView.delegate = self
  }

  private func layout() {
    view.addSubview(tableView)
    NSLayoutConstraint.activate([
      tableView.topAnchor.constraint(equalTo: view.topAnchor),
      tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      tableView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }

  private func updateState(_ state: RecentlyDeletedCategoriesViewModel.State) {
    switch state {
    case .searching:
      navigationController?.setToolbarHidden(true, animated: false)
      searchController.searchBar.isUserInteractionEnabled = true
    case .nothingSelected:
      navigationController?.setToolbarHidden(false, animated: false)
      recoverButton.title = LocalizedStrings.recoverAll
      deleteButton.title = LocalizedStrings.deleteAll
      searchController.searchBar.isUserInteractionEnabled = true
      navigationItem.rightBarButtonItem = nil
      tableView.indexPathsForSelectedRows?.forEach { tableView.deselectRow(at: $0, animated: true)}
    case .someSelected:
      navigationController?.setToolbarHidden(false, animated: false)
      recoverButton.title = LocalizedStrings.recover
      deleteButton.title = LocalizedStrings.delete
      searchController.searchBar.isUserInteractionEnabled = false
      navigationItem.rightBarButtonItem = clearButton
    }
  }

  // MARK: - Actions
  @objc private func clearButtonDidTap() {
    viewModel.cancelSelecting()
  }

  @objc private func recoverButtonDidTap() {
    viewModel.recoverSelectedCategories()
  }

  @objc private func deleteButtonDidTap() {
    viewModel.deleteSelectedCategories()
  }
}

// MARK: - UITableViewDataSource
extension RecentlyDeletedCategoriesViewController: UITableViewDataSource {
  func numberOfSections(in tableView: UITableView) -> Int {
    viewModel.filteredDataSource.count
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    viewModel.filteredDataSource[section].content.count
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: RecentlyDeletedTableViewCell.self, indexPath: indexPath)
    let category = viewModel.filteredDataSource[indexPath.section].content[indexPath.row]
    cell.configureWith(RecentlyDeletedTableViewCell.ViewModel(category))
    return cell
  }
}

// MARK: - UITableViewDelegate
extension RecentlyDeletedCategoriesViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard tableView.isEditing else {
      tableView.deselectRow(at: indexPath, animated: true)
      return
    }
    viewModel.selectCategory(at: indexPath)
  }

  func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
    guard tableView.isEditing else { return }
    guard let selectedIndexPaths = tableView.indexPathsForSelectedRows, !selectedIndexPaths.isEmpty else {
      viewModel.deselectAllCategories()
      return
    }
    viewModel.deselectCategory(at: indexPath)
  }

  func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
    let deleteAction = UIContextualAction(style: .destructive, title: LocalizedStrings.delete) { [weak self] (_, _, completion) in
      self?.viewModel.deleteCategory(at: indexPath)
      completion(true)
    }
    let recoverAction = UIContextualAction(style: .normal, title: LocalizedStrings.recover) { [weak self] (_, _, completion) in
      self?.viewModel.recoverCategory(at: indexPath)
      completion(true)
    }
    return UISwipeActionsConfiguration(actions: [deleteAction, recoverAction])
  }
}

// MARK: - UISearchBarDelegate
extension RecentlyDeletedCategoriesViewController: UISearchBarDelegate {
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(true, animated: true)
    viewModel.startSearching()
  }

  func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(false, animated: true)
  }

  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    searchBar.text = nil
    searchBar.resignFirstResponder()
    viewModel.cancelSearching()
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    viewModel.search(searchText)
  }
}
