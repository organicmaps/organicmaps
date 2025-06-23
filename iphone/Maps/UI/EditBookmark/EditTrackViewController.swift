import UIKit

final class EditTrackViewController: MWMTableViewController {
  private enum Sections: Int {
    case info
    case delete
    case count
  }
  
  private enum InfoSectionRows: Int {
    case title
    case color
    //case lineWidth // TODO: possible new section & ability - edit track line width
    case bookmarkGroup
    case count
  }

  enum TrackSource: Equatable {
    case track(trackId: MWMTrackID)
    case trackRecording
  }

  private var editingCompleted: (Bool) -> Void

  private var placePageData: PlacePageData?
  private let trackSource: TrackSource
  private let trackId: MWMTrackID
  private var trackTitle: String
  private var trackGroupTitle: String
  private var trackGroupId = FrameworkHelper.invalidCategoryId()
  private var trackColor: UIColor

  private let bookmarksManager = BookmarksManager.shared()

  @objc
  convenience init(trackId: MWMTrackID, editCompletion completion: @escaping (Bool) -> Void) {
    self.init(for: .track(trackId: trackId), editCompletion: completion)
  }

  init(for source: TrackSource, editCompletion completion: @escaping (Bool) -> Void) {
    self.trackSource = source
    switch source {
    case .track(let trackId):
      self.trackId = trackId

      let track = bookmarksManager.track(withId: trackId)
      self.trackTitle = track.trackName
      self.trackColor = track.trackColor

      let category = bookmarksManager.category(forTrackId: trackId)
      self.trackGroupId = category.categoryId
      self.trackGroupTitle = category.title

      self.editingCompleted = completion

    case .trackRecording:
      self.trackId = UInt64.max

      self.trackTitle = FrameworkHelper.generateTrackRecordingName()
      self.trackColor = FrameworkHelper.generateTrackRecordingColor()
      self.trackGroupId = FrameworkHelper.getDefaultTrackRecordingsCategory()
      let category = bookmarksManager.category(withId: self.trackGroupId)
      self.trackGroupTitle = category.title

      self.editingCompleted = completion
    }
    super.init(style: .grouped)
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    switch trackSource {
    case .track:
      updateTrackIfNeeded()
    case .trackRecording:
      break
    }
  }

  deinit {
    removeFromBookmarksManagerObserverList()
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    switch trackSource {
    case .track:
      title = L("track_title")
    case .trackRecording:
      title = "Save Track Recording"
      navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .cancel,
                                                         target: self,
                                                         action: #selector(close))
    }

    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .save,
                                                        target: self,
                                                        action: #selector(onSave))

    tableView.registerNib(cell: BookmarkTitleCell.self)
    tableView.registerNib(cell: MWMButtonCell.self)

    addToBookmarksManagerObserverList()
  }
    
  // MARK: - Table view data source
  
  override func numberOfSections(in tableView: UITableView) -> Int {
    Sections.count.rawValue
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch Sections(rawValue: section) {
    case .info:
      return InfoSectionRows.count.rawValue
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
        cell.configure(name: trackTitle, delegate: self, hint: L("placepage_track_name_hint"))
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
        return cell;
      default:
        fatalError()
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
      close()
    }
  }

  private func addToBookmarksManagerObserverList() {
    switch trackSource {
    case .track:
      bookmarksManager.add(self)
    case .trackRecording:
      break
    }
  }

  private func removeFromBookmarksManagerObserverList() {
    bookmarksManager.remove(self)
  }

  @objc private func onSave() {
    view.endEditing(true)
    switch trackSource {
    case .track:
      bookmarksManager.updateTrack(trackId, setGroupId: trackGroupId, color: trackColor, title: trackTitle)
      editingCompleted(true)
      close()
    case .trackRecording:
      let configuration = TrackSavingConfiguration(name: trackTitle, groupId: trackGroupId, color: trackColor)
      TrackRecordingManager.shared.stopAndSave(with: configuration) { [weak self] result in
        guard let self else { return }
        self.editingCompleted(result == .success ? true : false)
        self.close()
      }
    }
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
    let groupViewController = SelectBookmarkGroupViewController(groupName: trackGroupTitle, groupId: trackGroupId)
    groupViewController.delegate = self
    let navigationController = UINavigationController(rootViewController: groupViewController)
    present(navigationController, animated: true, completion: nil)
  }

  @objc
  private func close() {
    switch trackSource {
    case .track:
      goBack()
    case .trackRecording:
      dismiss(animated: true, completion: nil)
    }
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
      switch trackSource {
      case .track:
        bookmarksManager.deleteTrack(trackId)
      case .trackRecording:
        // TODO: Handle track recording deletion without saving.
        break
      }
      close()
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
extension EditTrackViewController: BookmarksObserver {
  func onBookmarksLoadFinished() {
    updateTrackIfNeeded()
  }

  func onBookmarksCategoryDeleted(_ groupId: MWMMarkGroupID) {
    if trackGroupId == groupId {
      close()
    }
  }
}
