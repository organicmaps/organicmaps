protocol IBookmarksListSectionViewModel {
  var numberOfItems: Int { get }
  var sectionTitle: String { get }
  var hasVisibilityButton: Bool { get }
  var canEdit: Bool { get }
}

protocol IBookmarksSectionViewModel: IBookmarksListSectionViewModel {
  var bookmarks: [IBookmarkViewModel] { get }
}

extension IBookmarksSectionViewModel {
  var numberOfItems: Int { bookmarks.count }
  var hasVisibilityButton: Bool { false }
  var canEdit: Bool { true }
}

protocol ITracksSectionViewModel: IBookmarksListSectionViewModel {
  var tracks: [ITrackViewModel] { get }
}

extension ITracksSectionViewModel {
  var numberOfItems: Int { tracks.count }
  var sectionTitle: String { L("tracks_title") }
  var hasVisibilityButton: Bool { false }
  var canEdit: Bool { true }
}

protocol ISubgroupsSectionViewModel: IBookmarksListSectionViewModel {
  var subgroups: [ISubgroupViewModel] { get }
}

extension ISubgroupsSectionViewModel {
  var numberOfItems: Int { subgroups.count }
  var hasVisibilityButton: Bool { true }
  var canEdit: Bool { false }
}

protocol IBookmarkViewModel {
  var bookmarkName: String { get }
  var subtitle: String { get }
  var image: UIImage { get }
}

protocol ITrackViewModel {
  var trackName: String { get }
  var subtitle: String { get }
  var image: UIImage { get }
}

protocol ISubgroupViewModel {
  var subgroupName: String { get }
  var subtitle: String { get }
  var isVisible: Bool { get }
}

protocol IBookmarksListMenuItem {
  var title: String { get }
  var destructive: Bool { get }
  var enabled: Bool { get }
  var action: () -> Void { get }
}

protocol IBookmarksListView: AnyObject {
  func setTitle(_ title: String)
  func setInfo(_ info: IBookmakrsListInfoViewModel)
  func setSections(_ sections: [IBookmarksListSectionViewModel])
  func setMoreItemTitle(_ itemTitle: String)
  func showMenu(_ items: [IBookmarksListMenuItem])
  func enableEditing(_ enable: Bool)
  func share(_ url: URL, completion: @escaping () -> Void)
  func showError(title: String, message: String)
}

final class BookmarksListViewController: MWMViewController {
  var presenter: IBookmarksListPresenter!

  private var sections: [IBookmarksListSectionViewModel]?
  private let cellStrategy = BookmarksListCellStrategy()

  private var canEdit = false

  @IBOutlet var tableView: UITableView!
  @IBOutlet var searchBar: UISearchBar!
  @IBOutlet var toolBar: UIToolbar!
  @IBOutlet var sortToolbarItem: UIBarButtonItem!
  @IBOutlet var moreToolbarItem: UIBarButtonItem!

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
    presenter.viewDidLoad()
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

  @IBAction func onSortItem(_ sender: UIBarButtonItem) {
    presenter.sort()
  }

  @IBAction func onMoreItem(_ sender: UIBarButtonItem) {
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
    48
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
                 commit editingStyle: UITableViewCell.EditingStyle,
                 forRowAt indexPath: IndexPath) {
    guard let section = sections?[indexPath.section] else { fatalError() }
    presenter.deleteBookmark(in: section, at: indexPath.row)
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
    shareController?.present(inParentViewController: self, anchorView: self.view)
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
