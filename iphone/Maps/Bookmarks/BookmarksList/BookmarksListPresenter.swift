protocol IBookmarksListPresenter {
  func viewDidLoad()
  func activateSearch()
  func deactivateSearch()
  func cancelSearch()
  func search(_ text: String)
  func sort()
  func more()
  func deleteBookmark(in section: IBookmarksListSectionViewModel, at index: Int)
  func selectItem(in section: IBookmarksListSectionViewModel, at index: Int)
  func checkItem(in section: IBookmarksListSectionViewModel, at index: Int, checked: Bool)
  func showDescription()
}

protocol BookmarksListDelegate: AnyObject {
  func bookmarksListDidDeleteGroup()
}

final class BookmarksListPresenter {
  private unowned let view: IBookmarksListView
  private let router: IBookmarksListRouter
  private let interactor: IBookmarksListInteractor
  private weak var delegate: BookmarksListDelegate?

  private let distanceFormatter = MeasurementFormatter()
  private let imperialUnits: Bool
  private let bookmarkGroup: BookmarkGroup

  init(view: IBookmarksListView,
       router: IBookmarksListRouter,
       delegate: BookmarksListDelegate?,
       interactor: IBookmarksListInteractor,
       imperialUnits: Bool) {
    self.view = view
    self.router = router
    self.delegate = delegate
    self.interactor = interactor
    self.imperialUnits = imperialUnits

    bookmarkGroup = interactor.getBookmarkGroup()
    distanceFormatter.unitOptions = [.providedUnit]
  }

  private func reload() {
    guard let sortingType = interactor.lastSortingType() else {
      setDefaultSections()
      return
    }
    sort(sortingType)
  }

  private func setDefaultSections() {
    interactor.resetSort()
    var sections: [IBookmarksListSectionViewModel] = []
    let tracks = bookmarkGroup.tracks.map { track in
      TrackViewModel(track, formattedDistance: formatDistance(Double(track.trackLengthMeters)))
    }
    if !tracks.isEmpty {
      sections.append(TracksSectionViewModel(tracks: tracks))
    }

    let collections = bookmarkGroup.collections.map { SubgroupViewModel($0) }
    if !collections.isEmpty {
      sections.append(SubgroupsSectionViewModel(title: L("collections"), subgroups: collections))
    }

    let categories = bookmarkGroup.categories.map { SubgroupViewModel($0)}
    if !categories.isEmpty {
      sections.append(SubgroupsSectionViewModel(title: L("categories"), subgroups: categories))
    }

    let bookmarks = mapBookmarks(bookmarkGroup.bookmarks)
    if !bookmarks.isEmpty {
      sections.append(BookmarksSectionViewModel(title: L("bookmarks"), bookmarks: bookmarks))
    }
    view.setSections(sections)
  }

  private func mapBookmarks(_ bookmarks: [Bookmark]) -> [BookmarkViewModel] {
    let location = LocationManager.lastLocation()
    return bookmarks.map {
      let formattedDistance: String?
      if let location = location {
        let distance = location.distance(from: CLLocation(latitude: $0.locationCoordinate.latitude,
                                                          longitude: $0.locationCoordinate.longitude))
        formattedDistance = formatDistance(distance)
      } else {
        formattedDistance = nil
      }
      return BookmarkViewModel($0, formattedDistance: formattedDistance)
    }
  }

  private func formatDistance(_ distance: Double) -> String {
    let unit = imperialUnits ? UnitLength.miles : UnitLength.kilometers
    let distanceInUnits = unit.converter.value(fromBaseUnitValue: distance)
    let measurement = Measurement(value: distanceInUnits.rounded(), unit: unit)
    return distanceFormatter.string(from: measurement)
  }

  private func showSortMenu() {
    var sortItems = interactor.availableSortingTypes(hasMyPosition: LocationManager.lastLocation() != nil)
      .map { sortingType -> BookmarksListMenuItem in
        switch sortingType {
        case .distance:
          return BookmarksListMenuItem(title: L("sort_distance"), action: { [weak self] in
            self?.sort(.distance)
            Statistics.logEvent(kStatBookmarksListSort, withParameters: [kStatOption : kStatSortByDistance])
          })
        case .date:
          return BookmarksListMenuItem(title: L("sort_date"), action: { [weak self] in
            self?.sort(.date)
            Statistics.logEvent(kStatBookmarksListSort, withParameters: [kStatOption : kStatSortByDate])
          })
        case .type:
          return BookmarksListMenuItem(title: L("sort_type"), action: { [weak self] in
            self?.sort(.type)
            Statistics.logEvent(kStatBookmarksListSort, withParameters: [kStatOption : kStatSortByType])
          })
        }
    }
    sortItems.append(BookmarksListMenuItem(title: L("sort_default"), action: { [weak self] in
      self?.setDefaultSections()
      Statistics.logEvent(kStatBookmarksListSort, withParameters: [kStatOption : kStatSortByDefault])
    }))
    view.showMenu(sortItems)
  }

  private func showMoreMenu() {
    var moreItems: [BookmarksListMenuItem] = []
    moreItems.append(BookmarksListMenuItem(title: L("sharing_options"), action: { [weak self] in
      guard let self = self else { return }
      self.router.sharingOptions(self.bookmarkGroup)
      Statistics.logEvent(kStatBookmarksListItemSettings, withParameters: [kStatOption : kStatSharingOptions])
    }))
    moreItems.append(BookmarksListMenuItem(title: L("search_show_on_map"), action: { [weak self] in
      self?.viewOnMap()
      Statistics.logEvent(kStatBookmarksListItemMoreClick, withParameters: [kStatOption : kStatViewOnMap])
    }))
    moreItems.append(BookmarksListMenuItem(title: L("list_settings"), action: { [weak self] in
      guard let self = self else { return }
      self.router.listSettings(self.bookmarkGroup, delegate: self)
      Statistics.logEvent(kStatBookmarksListItemMoreClick, withParameters: [kStatOption : kStatSettings])
    }))
    moreItems.append(BookmarksListMenuItem(title: L("export_file"), action: { [weak self] in
      self?.interactor.exportFile { (url, status) in
        switch status {
        case .success:
          guard let url = url else { fatalError() }
          self?.view.share(url) {
            self?.interactor.finishExportFile()
          }
        case .empty:
          self?.view.showError(title: L("bookmarks_error_title_share_empty"),
                               message: L("bookmarks_error_message_share_empty"))
        case .error:
          self?.view.showError(title: L("dialog_routing_system_error"),
                               message: L("bookmarks_error_message_share_general"))
        }
      }
      Statistics.logEvent(kStatBookmarksListItemMoreClick, withParameters: [kStatOption : kStatSendAsFile])
    }))
    moreItems.append(BookmarksListMenuItem(title: L("delete_list"),
                                           destructive: true,
                                           enabled: interactor.canDeleteGroup(),
                                           action: { [weak self] in
                                            self?.interactor.deleteBookmarksGroup()
                                            self?.delegate?.bookmarksListDidDeleteGroup()
                                            Statistics.logEvent(kStatBookmarksListItemMoreClick,
                                                                withParameters: [kStatOption : kStatDelete])

                                           }))
    view.showMenu(moreItems)
    Statistics.logEvent(kStatBookmarksListItemSettings, withParameters: [kStatOption : kStatMore])
  }

  private func viewOnMap() {
    interactor.viewOnMap()
    router.viewOnMap(bookmarkGroup)
  }

  private func sort(_ sortingType: BookmarksListSortingType) {
    interactor.sort(sortingType, location: LocationManager.lastLocation()) { [weak self] sortedSections in
      let sections = sortedSections.map { (bookmarksSection) -> IBookmarksListSectionViewModel in
        if let bookmarks = bookmarksSection.bookmarks, let self = self {
          return BookmarksSectionViewModel(title: bookmarksSection.sectionName, bookmarks: self.mapBookmarks(bookmarks))
        }
        if let tracks = bookmarksSection.tracks, let self = self {
          return TracksSectionViewModel(tracks: tracks.map { track in
            TrackViewModel(track, formattedDistance: self.formatDistance(Double(track.trackLengthMeters)))
          })
        }
        fatalError()
      }
      self?.view.setSections(sections)
    }
  }
}

extension BookmarksListPresenter: IBookmarksListPresenter {
  func viewDidLoad() {
    reload()
    view.setTitle(bookmarkGroup.title)
    view.setMoreItemTitle(bookmarkGroup.isEditable ? L("placepage_more_button") : L("view_on_map_bookmarks"))
    view.enableEditing(bookmarkGroup.isEditable)

    let info = BookmarksListInfo(title: bookmarkGroup.title,
                                 author: bookmarkGroup.author,
                                 hasDescription: bookmarkGroup.hasDescription,
                                 imageUrl: bookmarkGroup.imageUrl,
                                 hasLogo: bookmarkGroup.isLonelyPlanet)
    view.setInfo(info)
  }

  func activateSearch() {
    interactor.prepareForSearch()
  }

  func deactivateSearch() {

  }

  func cancelSearch() {
    reload()
  }

  func search(_ text: String) {
    interactor.search(text) { [weak self] in
      guard let self = self else { return }
      let bookmarks = self.mapBookmarks($0)
      self.view.setSections(bookmarks.isEmpty ? [] : [BookmarksSectionViewModel(title: L("bookmarks"),
                                                                                 bookmarks: bookmarks)])
      Statistics.logEvent(kStatBookmarksSearch, withParameters: [kStatFrom : kStatBookmarksList])
    }
  }

  func more() {
    if bookmarkGroup.isEditable {
      showMoreMenu()
    } else {
      viewOnMap()
    }
  }

  func sort() {
    showSortMenu()
  }

  func deleteBookmark(in section: IBookmarksListSectionViewModel, at index: Int) {
    guard let bookmarksSection = section as? BookmarksSectionViewModel else {
      fatalError("It's only possible to delete a bookmark")
    }
    guard let bookmark = bookmarksSection.bookmarks[index] as? BookmarkViewModel else { fatalError() }
    interactor.deleteBookmark(bookmark.bookmarkId)
    reload()
  }

  func selectItem(in section: IBookmarksListSectionViewModel, at index: Int) {
    switch section {
    case let bookmarksSection as IBookmarksSectionViewModel:
      let bookmark = bookmarksSection.bookmarks[index] as! BookmarkViewModel
      interactor.viewBookmarkOnMap(bookmark.bookmarkId)
      router.viewOnMap(bookmarkGroup)
      Statistics.logEvent(kStatEventName(kStatBookmarks, kStatShowOnMap))
      if bookmarkGroup.isGuide {
        Statistics.logEvent(kStatGuidesBookmarkSelect,
                            withParameters: [kStatServerId : bookmarkGroup.serverId],
                            with: .realtime)
      }
    case let tracksSection as ITracksSectionViewModel:
      let track = tracksSection.tracks[index] as! TrackViewModel
      interactor.viewTrackOnMap(track.trackId)
      router.viewOnMap(bookmarkGroup)
      if bookmarkGroup.isGuide {
        Statistics.logEvent(kStatGuidesTrackSelect,
                            withParameters: [kStatServerId : bookmarkGroup.serverId],
                            with: .realtime)
      }
    case let subgroupsSection as ISubgroupsSectionViewModel:
      let subgroup = subgroupsSection.subgroups[index] as! SubgroupViewModel
      router.showSubgroup(subgroup.groupId)
    default:
      fatalError("Wrong section type: \(section.self)")
    }
  }

  func checkItem(in section: IBookmarksListSectionViewModel, at index: Int, checked: Bool) {
    switch section {
    case let subgroupsSection as ISubgroupsSectionViewModel:
      let subgroup = subgroupsSection.subgroups[index] as! SubgroupViewModel
      interactor.setGroup(subgroup.groupId, visible: checked)
    default:
      fatalError("Wrong section type: \(section.self)")
    }
  }

  func showDescription() {
    router.showDescription(bookmarkGroup)
  }
}

extension BookmarksListPresenter: BookmarksSharingViewControllerDelegate {
  func didShareCategory() {
    let info = BookmarksListInfo(title: bookmarkGroup.title,
                                 author: bookmarkGroup.author,
                                 hasDescription: bookmarkGroup.hasDescription,
                                 imageUrl: bookmarkGroup.imageUrl,
                                 hasLogo: bookmarkGroup.isLonelyPlanet)
    view.setInfo(info)
  }
}

extension BookmarksListPresenter: CategorySettingsViewControllerDelegate {
  func categorySettingsController(_ viewController: CategorySettingsViewController, didEndEditing categoryId: MWMMarkGroupID) {
    let info = BookmarksListInfo(title: bookmarkGroup.title,
                                 author: bookmarkGroup.author,
                                 hasDescription: bookmarkGroup.hasDescription,
                                 imageUrl: bookmarkGroup.imageUrl,
                                 hasLogo: bookmarkGroup.isLonelyPlanet)
    view.setInfo(info)
  }

  func categorySettingsController(_ viewController: CategorySettingsViewController, didDelete categoryId: MWMMarkGroupID) {
    delegate?.bookmarksListDidDeleteGroup()
  }
}

fileprivate struct BookmarkViewModel: IBookmarkViewModel {
  let bookmarkId: MWMMarkID
  let bookmarkName: String
  let subtitle: String
  var image: UIImage {
    bookmarkColor.image(bookmarkIconName)
  }

  private let bookmarkColor: BookmarkColor
  private let bookmarkIconName: String

  init(_ bookmark: Bookmark, formattedDistance: String?) {
    bookmarkId = bookmark.bookmarkId
    bookmarkName = bookmark.bookmarkName
    bookmarkColor = bookmark.bookmarkColor
    bookmarkIconName = bookmark.bookmarkIconName
    subtitle = [formattedDistance, bookmark.bookmarkType].compactMap { $0 }.joined(separator: " • ")
  }
}

fileprivate struct TrackViewModel: ITrackViewModel {
  let trackId: MWMTrackID
  let trackName: String
  let subtitle: String
  var image: UIImage {
    circleImageForColor(trackColor, frameSize: 22)
  }

  private let trackColor: UIColor

  init(_ track: Track, formattedDistance: String) {
    trackId = track.trackId
    trackName = track.trackName
    subtitle = "\(L("length")) \(formattedDistance)"
    trackColor = track.trackColor
  }
}

fileprivate struct SubgroupViewModel: ISubgroupViewModel {
  let groupId: MWMMarkGroupID
  let subgroupName: String
  let subtitle: String
  let isVisible: Bool

  init(_ bookmarkGroup: BookmarkGroup) {
    groupId = bookmarkGroup.categoryId
    subgroupName = bookmarkGroup.title
    subtitle = bookmarkGroup.placesCountTitle()
    isVisible = bookmarkGroup.isVisible
  }
}

fileprivate struct BookmarksSectionViewModel: IBookmarksSectionViewModel {
  let sectionTitle: String
  let bookmarks: [IBookmarkViewModel]

  init(title: String, bookmarks: [IBookmarkViewModel]) {
    sectionTitle = title
    self.bookmarks = bookmarks
  }
}

fileprivate struct TracksSectionViewModel: ITracksSectionViewModel {
  let tracks: [ITrackViewModel]

  init(tracks: [ITrackViewModel]) {
    self.tracks = tracks
  }
}

fileprivate struct SubgroupsSectionViewModel: ISubgroupsSectionViewModel {
  let subgroups: [ISubgroupViewModel]
  let sectionTitle: String

  init(title: String, subgroups: [ISubgroupViewModel]) {
    sectionTitle = title
    self.subgroups = subgroups
  }
}

fileprivate struct BookmarksListMenuItem: IBookmarksListMenuItem {
  let title: String
  let destructive: Bool
  let enabled: Bool
  let action: () -> Void

  init(title: String, destructive: Bool = false, enabled: Bool = true, action: @escaping () -> Void) {
    self.title = title
    self.destructive = destructive
    self.enabled = enabled
    self.action = action
  }
}

fileprivate struct BookmarksListInfo: IBookmakrsListInfoViewModel {
  let title: String
  let author: String
  let hasDescription: Bool
  let imageUrl: URL?
  let hasLogo: Bool

  init(title: String, author: String, hasDescription:Bool, imageUrl: URL? = nil, hasLogo: Bool = false) {
    self.title = title
    self.author = author
    self.hasDescription = hasDescription
    self.imageUrl = imageUrl
    self.hasLogo = hasLogo
  }
}
