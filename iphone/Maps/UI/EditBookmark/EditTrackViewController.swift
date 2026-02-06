import UIKit

final class EditTrackViewController: MWMTableViewController {
  private enum Sections: Int {
    case info
    case description
    case delete
    case count
  }

  private enum InfoSectionRows: Int {
    case title
    case color
    // case lineWidth // TODO: possible new section & ability - edit track line width
    case bookmarkGroup
    case count
  }

  private var editingCompleted: (Bool) -> Void

  private var placePageData: PlacePageData?
  private let trackId: MWMTrackID
  private var trackTitle: String?
  private var trackGroupTitle: String?
  private var trackGroupId = FrameworkHelper.invalidCategoryId()
  private var trackColor: UIColor
  private var trackDescription: String?
  private var noteCell: MWMNoteCell?

  private let bookmarksManager = BookmarksManager.shared()

  @objc
  init(trackId: MWMTrackID, editCompletion completion: @escaping (Bool) -> Void) {
    self.trackId = trackId

    let track = bookmarksManager.track(withId: trackId)
    trackTitle = track.trackName
    trackColor = track.trackColor
    trackDescription = bookmarksManager.description(forTrackId: trackId)

    let category = bookmarksManager.category(forTrackId: trackId)
    trackGroupId = category.categoryId
    trackGroupTitle = category.title

    editingCompleted = completion
    super.init(style: .grouped)
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    updateTrackIfNeeded()
  }

  deinit {
    removeFromBookmarksManagerObserverList()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = L("track_title")
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .save,
                                                        target: self,
                                                        action: #selector(onSave))

    tableView.registerNib(cell: BookmarkTitleCell.self)
    tableView.registerNib(cell: MWMButtonCell.self)
    tableView.registerNib(cell: MWMNoteCell.self)

    addToBookmarksManagerObserverList()
  }

  // MARK: - Table view data source

  override func numberOfSections(in _: UITableView) -> Int {
    Sections.count.rawValue
  }

//  override func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
//    switch Sections(rawValue: section) {
//    case .info:
//      return InfoSectionRows.count.rawValue
//    case .delete:
//      return 1
//    default:
//      fatalError()
//    }
//  }
  
  
  override func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch Sections(rawValue: section) {
    case .info:
      return InfoSectionRows.count.rawValue
    case .description:
      return 1
    case .delete:
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
        cell.configure(name: trackTitle ?? "", delegate: self, hint: L("placepage_track_name_hint"))
        return cell
      case .color:
        let cell = tableView.dequeueDefaultCell(for: indexPath)
        cell.accessoryType = .disclosureIndicator
        cell.textLabel?.text = L("change_color")
        cell.imageView?.image = circleImageForColor(trackColor, frameSize: 28, diameter: 22)
        return cell
      case .bookmarkGroup:
        let cell = tableView.dequeueDefaultCell(for: indexPath)
        cell.textLabel?.text = trackGroupTitle
        cell.imageView?.image = UIImage(named: "ic_folder")
        cell.imageView?.setStyle(.black)
        cell.accessoryType = .disclosureIndicator
        return cell
      default:
        fatalError()
      }
      
      case .description:
          if let noteCell = noteCell {
            return noteCell
          } else {
            let cell = tableView.dequeueReusableCell(cell: MWMNoteCell.self, indexPath: indexPath)
            cell.config(with: self, noteText: trackDescription ?? "", placeholder: L("placepage_personal_notes_hint"))
            noteCell = cell
            return cell
          }
    case .delete:
      let cell = tableView.dequeueReusableCell(cell: MWMButtonCell.self, indexPath: indexPath)
      cell.configure(with: self, title: L("placepage_delete_track_button"), enabled: true)
      return cell
    default:
      fatalError()
    }
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    switch InfoSectionRows(rawValue: indexPath.row) {
    case .color:
      openColorPicker()
    case .bookmarkGroup:
      openGroupPicker()
    default:
      break
    }
  }

  // MARK: - Private

  private func updateTrackIfNeeded() {
    // TODO: Update the track content on the Edit screen instead of closing it when the track gets updated from cloud.
    if !bookmarksManager.hasTrack(trackId) {
      goBack()
    }
  }

  private func addToBookmarksManagerObserverList() {
    bookmarksManager.add(self)
  }

  private func removeFromBookmarksManagerObserverList() {
    bookmarksManager.remove(self)
  }

  @objc private func onSave() {
    view.endEditing(true)
    BookmarksManager.shared().updateTrack(trackId, setGroupId: trackGroupId, color: trackColor, title: trackTitle ?? "", description: trackDescription ?? "")
    editingCompleted(true)
    goBack()
  }

  private func updateColor(_ color: UIColor) {
    trackColor = color
    tableView.reloadRows(at: [IndexPath(row: InfoSectionRows.color.rawValue, section: Sections.info.rawValue)],
                         with: .none)
  }

  @objc private func openColorPicker() {
    ColorPicker.shared.present(from: self, pickerType: .defaultColorPicker(trackColor), completionHandler: { [weak self] color in
      self?.updateColor(color)
    })
  }

  private func openGroupPicker() {
    let groupViewController = SelectBookmarkGroupViewController(groupName: trackGroupTitle ?? "", groupId: trackGroupId)
    groupViewController.delegate = self
    let navigationController = UINavigationController(rootViewController: groupViewController)
    present(navigationController, animated: true, completion: nil)
  }
}

extension EditTrackViewController: BookmarkTitleCellDelegate {
  func didFinishEditingTitle(_ title: String) {
    trackTitle = title
  }
}

// MARK: - MWMButtonCellDelegate

extension EditTrackViewController: MWMButtonCellDelegate {
  func cellDidPressButton(_ cell: UITableViewCell) {
    guard let indexPath = tableView.indexPath(for: cell) else {
      fatalError("Invalid cell")
    }
    switch Sections(rawValue: indexPath.section) {
    case .info:
      break
    case .delete:
      bookmarksManager.deleteTrack(trackId)
      goBack()
    default:
      fatalError("Invalid section")
    }
  }
}

// MARK: - BookmarkColorViewControllerDelegate

extension EditTrackViewController: BookmarkColorViewControllerDelegate {
  func bookmarkColorViewController(_ viewController: BookmarkColorViewController, didSelect bookmarkColor: BookmarkColor) {
    viewController.dismiss(animated: true)
    updateColor(bookmarkColor.color)
  }
}

// MARK: - SelectBookmarkGroupViewControllerDelegate

extension EditTrackViewController: SelectBookmarkGroupViewControllerDelegate {
  func bookmarkGroupViewController(_ viewController: SelectBookmarkGroupViewController,
                                   didSelect groupTitle: String,
                                   groupId: MWMMarkGroupID) {
    viewController.dismiss(animated: true)
    trackGroupTitle = groupTitle
    trackGroupId = groupId
    tableView.reloadRows(at: [IndexPath(row: InfoSectionRows.bookmarkGroup.rawValue, section: Sections.info.rawValue)],
                         with: .none)
  }
}


// MARK: - BookmarksObserver
// MARK: - BookmarksObserver

extension EditTrackViewController: BookmarksObserver {
  func onBookmarksLoadFinished() {
    updateTrackIfNeeded()
  }

  func onBookmarksCategoryDeleted(_ groupId: MWMMarkGroupID) {
    if trackGroupId == groupId {
      goBack()
    }
  }
}

// MARK: - MWMNoteCellDelegate

extension EditTrackViewController: MWMNoteCellDelegate {
  func cell(_: MWMNoteCell, didChangeSizeAndText _: String) {
    UIView.setAnimationsEnabled(false)
    tableView.refresh()
    UIView.setAnimationsEnabled(true)
  }

  func cell(_: MWMNoteCell, didFinishEditingWithText text: String) {
    trackDescription = text
  }
}
