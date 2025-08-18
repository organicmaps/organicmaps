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
  private let bookmarkGroups = BookmarksManager.shared().sortedUserCategories()

  init(groupName: String, groupId: MWMMarkGroupID) {
    self.groupName = groupName
    self.groupId = groupId
    super.init(style: .grouped)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("bookmark_sets");
    navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(cancelButtonDidTap))
  }

  @objc private func cancelButtonDidTap() {
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
      return bookmarkGroups.count
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
      let bookmarkGroup = bookmarkGroups[indexPath.row]
      cell.textLabel?.text = bookmarkGroup.title
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
      let selectedGroup = bookmarkGroups[indexPath.row]
      delegate?.bookmarkGroupViewController(self, didSelect: selectedGroup.title, groupId: selectedGroup.categoryId)
    default:
      fatalError()
    }
  }

  private func createNewGroup() {
    alertController.presentCreateBookmarkCategoryAlert(withMaxCharacterNum: 60, minCharacterNum: 0) {
      [unowned self] name -> Bool in
      guard BookmarksManager.shared().checkCategoryName(name) else { return false }
      let newGroupId = BookmarksManager.shared().createCategory(withName: name)
      self.delegate?.bookmarkGroupViewController(self, didSelect: name, groupId: newGroupId)
      return true
    }
  }
}
