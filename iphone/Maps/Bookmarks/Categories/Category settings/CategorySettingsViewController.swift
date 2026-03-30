@objc protocol CategorySettingsViewControllerDelegate: AnyObject {
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didEndEditing categoryId: MWMMarkGroupID)
  func categorySettingsController(_ viewController: CategorySettingsViewController,
                                  didDelete categoryId: MWMMarkGroupID)
}

final class CategorySettingsViewController: MWMTableViewController {
  private enum Sections: Int {
    case info
    case description
    case color
    case delete
  }

  private enum ColorAction {
    case bookmarks
    case tracks

    var title: String {
      switch self {
      case .bookmarks: L("change_all_bookmarks_color")
      case .tracks: L("change_all_tracks_color")
      }
    }

    var toastMessage: String {
      switch self {
      case .bookmarks: L("toast_bookmarks_color_changed")
      case .tracks: L("toast_tracks_color_changed")
      }
    }
  }

  private enum InfoSectionRows: Int {
    case title
  }

  private let bookmarkGroup: BookmarkGroup
  private var noteCell: MWMNoteCell?
  private var changesMade = false
  private var newName: String?
  private var newAnnotation: String?

  private var colorActions: [ColorAction] {
    var actions: [ColorAction] = []
    if bookmarkGroup.bookmarksCount > 0 {
      actions.append(.bookmarks)
    }
    if bookmarkGroup.trackCount > 0 {
      actions.append(.tracks)
    }
    return actions
  }

  private var sections: [Sections] {
    var sections: [Sections] = [.info, .description]
    if !colorActions.isEmpty {
      sections.append(.color)
    }
    sections.append(.delete)
    return sections
  }

  @objc weak var delegate: CategorySettingsViewControllerDelegate?

  @objc init(bookmarkGroup: BookmarkGroup) {
    self.bookmarkGroup = bookmarkGroup
    super.init(style: .grouped)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = L("edit")
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .save,
                                                        target: self,
                                                        action: #selector(onSave))

    tableView.registerNib(cell: BookmarkTitleCell.self)
    tableView.registerNib(cell: MWMButtonCell.self)
    tableView.registerNib(cell: MWMNoteCell.self)
  }

  override func numberOfSections(in _: UITableView) -> Int {
    sections.count
  }

  override func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch sectionType(for: section) {
    case .info, .description, .delete:
      return 1
    case .color:
      return colorActions.count
    default:
      fatalError()
    }
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    switch sectionType(for: indexPath.section) {
    case .info:
      switch InfoSectionRows(rawValue: indexPath.row) {
      case .title:
        let cell = tableView.dequeueReusableCell(cell: BookmarkTitleCell.self, indexPath: indexPath)
        cell.configure(name: bookmarkGroup.title, delegate: self, hint: L("bookmarks_error_message_empty_list_name"))
        return cell
      default:
        fatalError()
      }
    case .description:
      if let noteCell = noteCell {
        return noteCell
      } else {
        let cell = tableView.dequeueReusableCell(cell: MWMNoteCell.self, indexPath: indexPath)
        cell.config(with: self, noteText: bookmarkGroup.detailedAnnotation,
                    placeholder: L("placepage_personal_notes_hint"))
        noteCell = cell
        return cell
      }
    case .color:
      let cell = tableView.dequeueDefaultCell(for: indexPath)
      cell.textLabel?.text = colorActions[indexPath.row].title
      cell.accessoryType = .disclosureIndicator
      return cell
    case .delete:
      let cell = tableView.dequeueReusableCell(cell: MWMButtonCell.self, indexPath: indexPath)
      cell.configure(with: self,
                     title: L("delete_list"),
                     enabled: BookmarksManager.shared().userCategoriesCount() > 1)
      return cell
    default:
      fatalError()
    }
  }

  @objc func onSave() {
    view.endEditing(true)
    if let newName = newName, !newName.isEmpty {
      BookmarksManager.shared().setCategory(bookmarkGroup.categoryId, name: newName)
      changesMade = true
    }

    if let newAnnotation = newAnnotation {
      BookmarksManager.shared().setCategory(bookmarkGroup.categoryId, description: newAnnotation)
      changesMade = true
    }

    delegate?.categorySettingsController(self, didEndEditing: bookmarkGroup.categoryId)
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)

    guard sectionType(for: indexPath.section) == .color else { return }
    openColorPicker(for: colorActions[indexPath.row])
  }

  private func sectionType(for section: Int) -> Sections? {
    guard sections.indices.contains(section) else { return nil }
    return sections[section]
  }

  private func openColorPicker(for colorAction: ColorAction) {
    ColorPicker.shared.present(from: self, pickerType: .bookmarkColorPicker(nil)) { [weak self] color in
      guard
        let self,
        let bookmarkColor = BookmarkColor.bookmarkColor(from: color)
      else {
        return
      }

      switch colorAction {
      case .bookmarks:
        BookmarksManager.shared().setCategory(self.bookmarkGroup.categoryId, bookmarksColor: bookmarkColor)
      case .tracks:
        BookmarksManager.shared().setCategory(self.bookmarkGroup.categoryId, tracksColor: bookmarkColor)
      }
      Toast.show(withText: colorAction.toastMessage, alignment: .top)
    }
  }
}

extension CategorySettingsViewController: BookmarkTitleCellDelegate {
  func didFinishEditingTitle(_ title: String) {
    newName = title
  }
}

extension CategorySettingsViewController: MWMNoteCellDelegate {
  func cell(_: MWMNoteCell, didChangeSizeAndText _: String) {
    UIView.setAnimationsEnabled(false)
    tableView.refresh()
    UIView.setAnimationsEnabled(true)
    tableView.scrollToRow(at: IndexPath(item: 0, section: Sections.description.rawValue),
                          at: .bottom,
                          animated: true)
  }

  func cell(_: MWMNoteCell, didFinishEditingWithText text: String) {
    newAnnotation = text
  }
}

extension CategorySettingsViewController: MWMButtonCellDelegate {
  func cellDidPressButton(_: UITableViewCell) {
    BookmarksManager.shared().deleteCategory(bookmarkGroup.categoryId)
    delegate?.categorySettingsController(self, didDelete: bookmarkGroup.categoryId)
  }
}
