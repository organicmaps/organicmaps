import Foundation

enum BookmarksListVisibilityButtonState {
  case hidden
  case hideAll
  case showAll
}

protocol IBookmarksListSectionViewModel {
  var numberOfItems: Int { get }
  var sectionTitle: String { get }
  var visibilityButtonState: BookmarksListVisibilityButtonState { get }
  var canEdit: Bool { get }
}

protocol IBookmarksSectionViewModel: IBookmarksListSectionViewModel {
  var bookmarks: [IBookmarkViewModel] { get }
}

protocol ITracksSectionViewModel: IBookmarksListSectionViewModel {
  var tracks: [ITrackViewModel] { get }
}

protocol ISubgroupsSectionViewModel: IBookmarksListSectionViewModel {
  var subgroups: [ISubgroupViewModel] { get }
  var type: BookmarkGroupType { get }
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
  func setInfo(_ info: IBookmarksListInfoViewModel)
  func setSections(_ sections: [IBookmarksListSectionViewModel])
  func setMoreItemTitle(_ itemTitle: String)
  func showMenu(_ items: [IBookmarksListMenuItem])
  func enableEditing(_ enable: Bool)
  func share(_ url: URL, completion: @escaping () -> Void)
  func showError(title: String, message: String)
}

protocol IBookmarksListPresenter {
  func viewDidLoad()
  func viewDidAppear()
  func activateSearch()
  func deactivateSearch()
  func cancelSearch()
  func search(_ text: String)
  func sort()
  func more()
  func deleteItem(in section: IBookmarksListSectionViewModel, at index: Int)
  func moveItem(in section: IBookmarksListSectionViewModel, at index: Int)
  func editItem(in section: IBookmarksListSectionViewModel, at index: Int)
  func selectItem(in section: IBookmarksListSectionViewModel, at index: Int)
  func checkItem(in section: IBookmarksListSectionViewModel, at index: Int, checked: Bool)
  func toggleVisibility(in section: IBookmarksListSectionViewModel)
  func showDescription()
}

enum BookmarksListSortingType {
  case distance
  case date
  case type
}

protocol IBookmarksListInteractor {
  func getBookmarkGroup() -> BookmarkGroup
  func hasDescription() -> Bool
  func prepareForSearch()
  func search(_ text: String, completion: @escaping ([Bookmark]) -> Void)
  func availableSortingTypes(hasMyPosition: Bool) -> [BookmarksListSortingType]
  func viewOnMap()
  func viewBookmarkOnMap(_ bookmarkId: MWMMarkID)
  func viewTrackOnMap(_ trackId: MWMTrackID)
  func setGroup(_ groupId: MWMMarkGroupID, visible: Bool)
  func sort(_ sortingType: BookmarksListSortingType,
            location: CLLocation?,
            completion: @escaping ([BookmarksSection]) -> Void)
  func resetSort()
  func lastSortingType() -> BookmarksListSortingType?
  func deleteBookmark(_ bookmarkId: MWMMarkID)
  func deleteTrack(_ trackId: MWMTrackID)
  func moveBookmark(_ bookmarkId: MWMMarkID, toGroupId: MWMMarkGroupID)
  func moveTrack(_ trackId: MWMTrackID, toGroupId: MWMMarkGroupID)
  func updateBookmark(_ bookmarkId: MWMMarkID, setGroupId groupId: MWMMarkGroupID, title: String, color: BookmarkColor, description: String)
  func updateTrack(_ trackId: MWMTrackID, setGroupId groupId: MWMMarkGroupID)
  func deleteBookmarksGroup()
  func canDeleteGroup() -> Bool
  func exportFile(_ completion: @escaping (URL?, ExportFileStatus) -> Void)
  func finishExportFile()
}

protocol IBookmarksListRouter {
  func listSettings(_ bookmarkGroup: BookmarkGroup, delegate: CategorySettingsViewControllerDelegate?)
  func viewOnMap(_ bookmarkGroup: BookmarkGroup)
  func showDescription(_ bookmarkGroup: BookmarkGroup)
  func showSubgroup(_ subgroupId: MWMMarkGroupID)
  func selectGroup(currentGroupName groupName: String,
                   currentGroupId groupId: MWMMarkGroupID,
                   delegate: SelectBookmarkGroupViewControllerDelegate?)
  func editBookmark(bookmarkId: MWMMarkID, completion: @escaping (Bool) -> Void)
  func editTrack(trackId: MWMTrackID, completion: @escaping (Bool) -> Void)
}

protocol IBookmarksListInfoViewModel {
  var title: String { get }
  var description: String { get }
  var hasDescription: Bool { get }
  var isHtmlDescription: Bool { get }
  var imageUrl: URL? { get }
}
