import UIKit

@objc(MWMEditBookmarkController)
final class EditBookmarkViewController: MWMTableViewController {
  private enum Sections: Int {
    case info
    case description
    case delete
    case count
  }

  private enum InfoSectionRows: Int {
    case title
    case color
    case bookmarkGroup
    case count
  }

  @objc var placePageData: PlacePageData!

  private var noteCell: MWMNoteCell?
  private var bookmarkTitle: String?
  private var bookmarkDescription: String?
  private var bookmarkGroupTitle: String?
  private var bookmarkId = FrameworkHelper.invalidBookmarkId()
  private var bookmarkGroupId = FrameworkHelper.invalidCategoryId()
  private var newBookmarkGroupId = FrameworkHelper.invalidCategoryId()
  private var bookmarkColor: BookmarkColor!

  override func viewDidLoad() {
    super.viewDidLoad()

    guard placePageData != nil, let bookmarkData = placePageData.bookmarkData else {
      fatalError("placePageData and bookmarkData can't be nil")
    }

    bookmarkTitle = placePageData.previewData.title
    bookmarkDescription = bookmarkData.bookmarkDescription
    bookmarkGroupTitle = bookmarkData.bookmarkCategory
    bookmarkId = bookmarkData.bookmarkId
    bookmarkGroupId = bookmarkData.bookmarkGroupId
    bookmarkColor = bookmarkData.color

    title = L("bookmark").capitalized
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .save,
                                                        target: self,
                                                        action: #selector(onSave))

    tableView.registerNib(cell: BookmarkTitleCell.self)
    tableView.registerNib(cell: MWMButtonCell.self)
    tableView.registerNib(cell: MWMNoteCell.self)
  }

  // MARK: - Table view data source

  override func numberOfSections(in tableView: UITableView) -> Int {
    Sections.count.rawValue
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch Sections(rawValue: section) {
    case .info:
      return InfoSectionRows.count.rawValue
    case .description, .delete:
      return 1
    default:
      fatalError()
    }
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    switch Sections(rawValue: indexPath.section) {
    case .info:
      switch InfoSectionRows(rawValue: indexPath.row) {
      case .title:
        let cell = tableView.dequeueReusableCell(cell: BookmarkTitleCell.self, indexPath: indexPath)
        cell.configure(name: bookmarkTitle ?? "", delegate: self)
        return cell
      case .color:
        let cell = tableView.dequeueDefaultCell(for: indexPath)
        cell.accessoryType = .disclosureIndicator
        cell.textLabel?.text = bookmarkColor.title
        cell.imageView?.image = circleImageForColor(bookmarkColor.color, frameSize: 28, diameter: 22, iconName: "ic_bm_none")
        return cell
      case .bookmarkGroup:
        let cell = tableView.dequeueDefaultCell(for: indexPath)
        cell.textLabel?.text = bookmarkGroupTitle
        cell.imageView?.image = UIImage(named: "ic_folder")
        cell.imageView?.styleName = "MWMBlack";
        cell.accessoryType = .disclosureIndicator;
        return cell;
      default:
        fatalError()
      }
    case .description:
      if let noteCell = noteCell {
        return noteCell
      } else {
        let cell = tableView.dequeueReusableCell(cell: MWMNoteCell.self, indexPath: indexPath)
        cell.config(with: self, noteText: bookmarkDescription ?? "", placeholder: L("placepage_personal_notes_hint"))
        noteCell = cell
        return cell
      }
    case .delete:
      let cell = tableView.dequeueReusableCell(cell: MWMButtonCell.self, indexPath: indexPath)
      cell.configure(with: self, title: L("placepage_delete_bookmark_button"), enabled: true)
      return cell
    default:
      fatalError()
    }
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    switch InfoSectionRows(rawValue: indexPath.row) {
    case .color:
      let colorViewController = BookmarkColorViewController(bookmarkColor: bookmarkColor)
      colorViewController.delegate = self
      navigationController?.pushViewController(colorViewController, animated: true)
    case .bookmarkGroup:
      let groupViewController = SelectBookmarkGroupViewController(groupName: bookmarkGroupTitle ?? "", groupId: bookmarkGroupId)
      groupViewController.delegate = self
      navigationController?.pushViewController(groupViewController, animated: true)
    default:
      break
    }
  }

  // MARK: - Private

  @objc private func onSave() {
    view.endEditing(true)

    BookmarksManager.shared().updateBookmark(bookmarkId,
                                             setGroupId: bookmarkGroupId,
                                             title: bookmarkTitle ?? "",
                                             color: bookmarkColor,
                                             description: bookmarkDescription ?? "")
    FrameworkHelper.updatePlacePageData()
    placePageData.updateBookmarkStatus()
    goBack()
  }
}

extension EditBookmarkViewController: BookmarkTitleCellDelegate {
  func didFinishEditingTitle(_ title: String) {
    bookmarkTitle = title
  }
}

extension EditBookmarkViewController: MWMNoteCellDelegate {
  func cell(_ cell: MWMNoteCell, didChangeSizeAndText text: String) {
    UIView.setAnimationsEnabled(false)
    tableView.refresh()
    UIView.setAnimationsEnabled(true)
    tableView.scrollToRow(at: IndexPath(item: 0, section: Sections.description.rawValue),
                          at: .bottom,
                          animated: true)
  }

  func cell(_ cell: MWMNoteCell, didFinishEditingWithText text: String) {
    bookmarkDescription = text
  }
}

extension EditBookmarkViewController: MWMButtonCellDelegate {
  func cellDidPressButton(_ cell: UITableViewCell) {
    BookmarksManager.shared().deleteBookmark(bookmarkId)
    FrameworkHelper.updatePlacePageData()
    placePageData.updateBookmarkStatus()
    goBack()
  }
}

extension EditBookmarkViewController: BookmarkColorViewControllerDelegate {
  func bookmarkColorViewController(_ viewController: BookmarkColorViewController, didSelect color: BookmarkColor) {
    goBack()
    bookmarkColor = color
    tableView.reloadRows(at: [IndexPath(row: InfoSectionRows.color.rawValue, section: Sections.info.rawValue)],
                         with: .none)
  }
}

extension EditBookmarkViewController: SelectBookmarkGroupViewControllerDelegate {
  func bookmarkGroupViewController(_ viewController: SelectBookmarkGroupViewController,
                                   didSelect groupTitle: String,
                                   groupId: MWMMarkGroupID) {
    goBack()
    bookmarkGroupTitle = groupTitle
    bookmarkGroupId = groupId
    tableView.reloadRows(at: [IndexPath(row: InfoSectionRows.bookmarkGroup.rawValue, section: Sections.info.rawValue)],
                         with: .none)
  }
}
