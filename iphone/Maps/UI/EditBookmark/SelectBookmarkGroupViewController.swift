import UIKit

protocol SelectBookmarkGroupViewControllerDelegate: AnyObject {
  func bookmarkGroupViewController(_ viewController: SelectBookmarkGroupViewController,
                                   didSelect groupTitle: String,
                                   groupId: MWMMarkGroupID)
}

final class SelectBookmarkGroupViewController: MWMTableViewController {
  private enum Sections: Int {
    case addGroup
    case groups
    case count
  }

  weak var delegate: SelectBookmarkGroupViewControllerDelegate?
  private let groupName: String
  private let groupId: MWMMarkGroupID
  private let bookmarkGroups: [NormalizedBookmarkGroup]
  private var filteredGroups: [NormalizedBookmarkGroup] = []
  private var isSearching = false
  private var currentGroups: [NormalizedBookmarkGroup] {
    isSearching ? filteredGroups : bookmarkGroups
  }
  private let searchController = UISearchController(searchResultsController: nil)

  private struct NormalizedBookmarkGroup {
    let categoryId: MWMMarkGroupID
    let title: String
    let normalizedTitle: String

    init(group: BookmarkGroup) {
      self.categoryId = group.categoryId
      self.title = group.title
      self.normalizedTitle = group.title.normalizedAndSimplified
    }
  }

  init(groupName: String, groupId: MWMMarkGroupID) {
    self.groupName = groupName
    self.groupId = groupId
    self.bookmarkGroups = BookmarksManager.shared().sortedUserCategories().map(NormalizedBookmarkGroup.init)
    super.init(style: .grouped)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
  }

  private func setupView() {
    title = L("bookmark_sets");
    navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(cancelButtonDidTap))

    searchController.searchBar.placeholder = L("search")
    searchController.obscuresBackgroundDuringPresentation = false
    searchController.hidesNavigationBarDuringPresentation = false
    searchController.searchBar.delegate = self
    searchController.searchBar.applyTheme()
    navigationItem.searchController = searchController
    navigationItem.hidesSearchBarWhenScrolling = false
  }

  @objc private func cancelButtonDidTap() {
    searchController.isActive = false
    dismiss(animated: true, completion: nil)
  }

  override func numberOfSections(in tableView: UITableView) -> Int {
    Sections.count.rawValue
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch Sections(rawValue: section) {
    case .addGroup:
      return 1
    case .groups:
      return currentGroups.count
    default:
      fatalError()
    }
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueDefaultCell(for: indexPath)
    switch Sections(rawValue: indexPath.section) {
    case .addGroup:
      cell.textLabel?.text = L("add_new_set")
      cell.accessoryType = .disclosureIndicator
    case .groups:
      let bookmarkGroup = currentGroups[indexPath.row]
      cell.textLabel?.text = bookmarkGroup.title
      cell.textLabel?.numberOfLines = 3
      cell.accessoryType = bookmarkGroup.categoryId == groupId ? .checkmark : .none
    default:
      fatalError()
    }
    return cell
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    switch Sections(rawValue: indexPath.section) {
    case .addGroup:
      createNewGroup()
    case .groups:
      searchController.isActive = false
      let selectedGroup = currentGroups[indexPath.row]
      delegate?.bookmarkGroupViewController(self, didSelect: selectedGroup.title, groupId: selectedGroup.categoryId)
    default:
      fatalError()
    }
  }

  private func createNewGroup() {
    alertController.presentCreateBookmarkCategoryAlert(withMaxCharacterNum: 60, minCharacterNum: 0) {
      [unowned self] name -> Bool in
      guard BookmarksManager.shared().checkCategoryName(name) else { return false }
      searchController.isActive = false
      let newGroupId = BookmarksManager.shared().createCategory(withName: name)
      self.delegate?.bookmarkGroupViewController(self, didSelect: name, groupId: newGroupId)
      return true
    }
  }

  private func applyFilter(for text: String) {
    let normalizedText = text.normalizedAndSimplified
    isSearching = !normalizedText.isEmpty
    if isSearching {
      filteredGroups = bookmarkGroups.filter { $0.normalizedTitle.contains(normalizedText) }
    } else {
      filteredGroups.removeAll(keepingCapacity: false)
    }
    tableView.reloadSections(IndexSet(integer: Sections.groups.rawValue), with: .automatic)
  }
}

// MARK: - UISearchBarDelegate

extension SelectBookmarkGroupViewController: UISearchBarDelegate {
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(true, animated: true)
  }

  func searchBarTextDidEndEditing(_ searchBar: UISearchBar) {
    searchBar.setShowsCancelButton(false, animated: true)
  }

  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    searchBar.text = nil
    searchBar.resignFirstResponder()
    applyFilter(for: "")
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    applyFilter(for: searchText)
  }
}

private extension String {
  var normalizedAndSimplified: String { (self as NSString).normalizedAndSimplified() }
}
