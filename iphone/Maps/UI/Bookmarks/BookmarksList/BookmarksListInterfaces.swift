import Foundation

enum BookmarkToolbarButtonSource {
  case sort
  case more
}

enum GroupReloadingResult {
  case success
  case notFound
}

enum BookmarksListItemId: Hashable {
  case bookmark(MWMMarkID)
  case track(MWMTrackID)
}

protocol IBookmarksListSectionViewModel {
  var numberOfItems: Int { get }
  var sectionTitle: String { get }
  var canEdit: Bool { get }
  var editableItems: [IBookmarksListItemViewModel] { get }
}

protocol IBookmarksSectionViewModel: IBookmarksListSectionViewModel {
  var bookmarks: [IBookmarksListItemViewModel] { get }
}

protocol ITracksSectionViewModel: IBookmarksListSectionViewModel {
  var tracks: [IBookmarksListItemViewModel] { get }
}

protocol IBookmarksListItemViewModel {
  var itemId: BookmarksListItemId { get }
  var name: String { get }
  var subtitle: String { get }
  var image: UIImage { get }
  var colorDidTapAction: ((_ anchor: UIView?) -> Void)? { get }
}

protocol IBookmarksListMenuItem {
  var title: String { get }
  var destructive: Bool { get }
  var enabled: Bool { get }
  var action: () -> Void { get }
}

protocol IBookmarksListView: AnyObject {
  func setInfo(_ info: IBookmarksListInfoViewModel)
  func setSections(_ sections: [IBookmarksListSectionViewModel])
  func showMenu(_ items: [IBookmarksListMenuItem], from source: BookmarkToolbarButtonSource)
  func showColorPicker(anchor: UIView?, currentColor: UIColor?, _ completion: ((UIColor) -> Void)?)
  func showBatchColorPicker(_ completion: ((UIColor) -> Void)?)
  func finishEditing()
  func enableEditing(_ enable: Bool)
  func share(_ url: URL, displayName: String, completion: @escaping () -> Void)
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
  func editCategory()
  func deleteItems(with itemIds: Set<BookmarksListItemId>)
  func moveItems(with itemIds: Set<BookmarksListItemId>)
  func changeColor(of itemIds: Set<BookmarksListItemId>)
  func editItem(in section: IBookmarksListSectionViewModel, at index: Int)
  func selectItem(in section: IBookmarksListSectionViewModel, at index: Int)
  func showDescription()
}

enum BookmarksListSortingType {
  case distance
  case date
  case type
  case name
}

protocol IBookmarksListInteractor {
  var onCategoryReload: ((GroupReloadingResult) -> Void)? { get set }

  func reloadCategory()
  func getBookmarkGroup() -> BookmarkGroup
  func prepareForSearch()
  func search(_ text: String, completion: @escaping ([Bookmark]) -> Void)
  func availableSortingTypes(hasMyPosition: Bool) -> [BookmarksListSortingType]
  func viewOnMap()
  func viewBookmarkOnMap(_ bookmarkId: MWMMarkID)
  func viewTrackOnMap(_ trackId: MWMTrackID)
  func sort(_ sortingType: BookmarksListSortingType,
            location: CLLocation?,
            completion: @escaping ([BookmarksSection]) -> Void)
  func resetSort()
  func lastSortingType() -> BookmarksListSortingType?
  func deleteItems(with itemIds: Set<BookmarksListItemId>)
  func moveItems(with itemIds: Set<BookmarksListItemId>, toGroupId: MWMMarkGroupID)
  func setColor(_ color: UIColor, for itemIds: Set<BookmarksListItemId>)
  func deleteBookmarksGroup()
  func canDeleteGroup() -> Bool
  func exportFile(fileType: FileType, completion: @escaping SharingResultCompletionHandler)
  func finishExportFile()
}

protocol IBookmarksListRouter {
  func listSettings(_ bookmarkGroup: BookmarkGroup, delegate: CategorySettingsViewControllerDelegate?)
  func viewOnMap(_ bookmarkGroup: BookmarkGroup)
  func showDescription(_ bookmarkGroup: BookmarkGroup)
  func selectGroup(currentGroupId groupId: MWMMarkGroupID,
                   delegate: SelectBookmarkGroupViewControllerDelegate?)
  func editBookmark(bookmarkId: MWMMarkID, completion: @escaping (Bool) -> Void)
  func editTrack(trackId: MWMTrackID, completion: @escaping (Bool) -> Void)
  func goBack()
}

protocol IBookmarksListInfoViewModel {
  var title: String { get }
  var description: String { get }
  var hasDescription: Bool { get }
  var isHtmlDescription: Bool { get }
  var imageUrl: URL? { get }
}
