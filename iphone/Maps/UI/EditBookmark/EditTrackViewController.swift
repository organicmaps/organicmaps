import UIKit

final class EditTrackViewController: MWMTableViewController {
  private enum Sections: Int {
    case info
    case delete
    case count
  }
  
  private enum InfoSectionRows: Int {
    case title
    //case color // TODO: need to know which layer color should be changed in track
    //case lineWidth // TODO: possible new section & ability - edit track line width
    case bookmarkGroup
    case count
  }
  
  private var editingCompleted: (Bool) -> Void

  private let trackId: MWMTrackID
  private var trackTitle: String?
  private var trackGroupTitle: String?
  private var trackGroupId = FrameworkHelper.invalidCategoryId()
  //private var trackColor: BookmarkColor // or add TrackColor type

  private let bookmarksManager = BookmarksManager.shared()
  
  init(trackId: MWMTrackID, editCompletion completion: @escaping (Bool) -> Void) {
    self.trackId = trackId
    
    let bm = BookmarksManager.shared()
    let track = bm.track(withId: trackId)
    
    trackTitle = track.trackName
    //trackColor = ....

    let category = bm.category(forTrackId: trackId)
    trackGroupId = category.categoryId
    trackGroupTitle = category.title
    

    editingCompleted = completion

    super.init(style: .grouped)
  }
  
  required init?(coder: NSCoder) {
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
        cell.configure(name: trackTitle ?? "", delegate: self, hint: L("placepage_track_name_hint"))
        return cell
//      case .color:
//        let cell = tableView.dequeueDefaultCell(for: indexPath)
//        cell.accessoryType = .disclosureIndicator
//        cell.textLabel?.text = trackColor.title
//        cell.imageView?.image = circleImageForColor(trackColor.color, frameSize: 28, diameter: 22, iconName: "ic_bm_none")
//        return cell
      case .bookmarkGroup:
        let cell = tableView.dequeueDefaultCell(for: indexPath)
        cell.textLabel?.text = trackGroupTitle
        cell.imageView?.image = UIImage(named: "ic_folder")
        cell.imageView?.styleName = "MWMBlack";
        cell.accessoryType = .disclosureIndicator;
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
    switch InfoSectionRows(rawValue: indexPath.row) {
//    case .color:
//      let colorViewController = BookmarkColorViewController(bookmarkColor: trackColor)
//      colorViewController.delegate = self
//      navigationController?.pushViewController(colorViewController, animated: true)
    case .bookmarkGroup:
      let groupViewController = SelectBookmarkGroupViewController(groupName: trackGroupTitle ?? "", groupId: trackGroupId)
      groupViewController.delegate = self
      navigationController?.pushViewController(groupViewController, animated: true)
    default:
      break
    }
  }
  
  // MARK: - Private
  
  @objc private func onSave() {
    view.endEditing(true)
    
    BookmarksManager.shared().updateTrack(trackId, setGroupId: trackGroupId, title: trackTitle ?? "")
    editingCompleted(true)
    goBack()
  }
}

extension EditTrackViewController: BookmarkTitleCellDelegate {
  func didFinishEditingTitle(_ title: String) {
    trackTitle = title
  }
}

extension EditTrackViewController: MWMButtonCellDelegate {
  func cellDidPressButton(_ cell: UITableViewCell) {
    bookmarksManager.deleteTrack(trackId)
    goBack()
  }
}

//extension EditTrackViewController: BookmarkColorViewControllerDelegate {
//  func bookmarkColorViewController(_ viewController: BookmarkColorViewController, didSelect color: BookmarkColor) {
//    goBack()
//    trackColor = color
//    tableView.reloadRows(at: [IndexPath(row: InfoSectionRows.color.rawValue, section: Sections.info.rawValue)],
//                         with: .none)
//  }
//}

extension EditTrackViewController: SelectBookmarkGroupViewControllerDelegate {
  func bookmarkGroupViewController(_ viewController: SelectBookmarkGroupViewController,
                                   didSelect groupTitle: String,
                                   groupId: MWMMarkGroupID) {
    goBack()
    trackGroupTitle = groupTitle
    trackGroupId = groupId
    tableView.reloadRows(at: [IndexPath(row: InfoSectionRows.bookmarkGroup.rawValue, section: Sections.info.rawValue)],
                         with: .none)
  }
}
