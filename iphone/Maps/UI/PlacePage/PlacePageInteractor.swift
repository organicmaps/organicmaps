protocol PlacePageInteractorProtocol: AnyObject {
  func viewWillDisappear()
  func updateVisibleAreaInsets(_ insets: UIEdgeInsets, updatingViewport: Bool)
  func close()
}

class PlacePageInteractor: NSObject {
  var presenter: PlacePagePresenterProtocol?
  weak var trackActivePointPresenter: TrackActivePointPresenter?

  private let bookmarksManager = BookmarksManager.shared()
  private let trackRecordingManager = TrackRecordingManager.shared
  private var placePageData: PlacePageData

  init(data: PlacePageData) {
    self.placePageData = data
    super.init()
    addToBookmarksManagerObserverList()
    subscribeOnTrackActivePointUpdatesIfNeeded()
  }

  deinit {
    removeFromBookmarksManagerObserverList()
  }

  private func updatePlacePageIfNeeded() {
    func updatePlacePage() {
      FrameworkHelper.updatePlacePageData()
    }

    switch placePageData.objectType {
    case .POI, .trackRecording:
      break
    case .bookmark:
      guard let bookmarkData = placePageData.bookmarkData, bookmarksManager.hasBookmark(bookmarkData.bookmarkId) else {
        presenter?.close()
        return
      }
      updatePlacePage()
    case .track:
      guard let trackData = placePageData.trackData, bookmarksManager.hasTrack(trackData.trackId) else {
        presenter?.close()
        return
      }
      updatePlacePage()
    @unknown default:
      fatalError("Unknown object type")
    }
  }

  private func subscribeOnTrackActivePointUpdatesIfNeeded() {
    unsubscribeFromTrackActivePointUpdates()
    guard placePageData.objectType == .track, let trackData = placePageData.trackData else { return }
    bookmarksManager.setElevationActivePointChanged(trackData.trackId) { [weak self] distance in
      self?.trackActivePointPresenter?.updateActivePointDistance(distance)
      trackData.updateActivePointDistance(distance)
    }
    bookmarksManager.setElevationMyPositionChanged(trackData.trackId) { [weak self] distance in
      self?.trackActivePointPresenter?.updateMyPositionDistance(distance)
    }
  }

  private func unsubscribeFromTrackActivePointUpdates() {
    bookmarksManager.resetElevationActivePointChanged()
    bookmarksManager.resetElevationMyPositionChanged()
  }

  private func addToBookmarksManagerObserverList() {
    bookmarksManager.add(self)
  }

  private func removeFromBookmarksManagerObserverList() {
    bookmarksManager.remove(self)
  }
}

extension PlacePageInteractor: PlacePageInteractorProtocol {
  func viewWillDisappear() {
    unsubscribeFromTrackActivePointUpdates()
  }

  func updateVisibleAreaInsets(_ insets: UIEdgeInsets, updatingViewport: Bool) {
    presenter?.updateVisibleAreaInsets(insets, updatingViewport: updatingViewport)
  }

  func close() {
    presenter?.close()
  }
}

// MARK: - PlacePageInfoViewControllerDelegate

extension PlacePageInteractor: PlacePageInfoViewControllerDelegate {
  var shouldShowOpenInApp: Bool {
    !OpenInApplication.availableApps.isEmpty
  }

  func didPressCall(to phone: PlacePagePhone) {
    MWMPlacePageManagerHelper.call(phone)
  }

  func didPressWebsite() {
    MWMPlacePageManagerHelper.openWebsite(placePageData)
  }

  func didPressWebsiteMenu() {
    MWMPlacePageManagerHelper.openWebsiteMenu(placePageData)
  }
  
  func didPressWikimediaCommons() {
    MWMPlacePageManagerHelper.openWikimediaCommons(placePageData)
  }
  
  func didPressFacebook() {
    MWMPlacePageManagerHelper.openFacebook(placePageData)
  }
  
  func didPressInstagram() {
    MWMPlacePageManagerHelper.openInstagram(placePageData)
  }

  func didPressTwitter() {
    MWMPlacePageManagerHelper.openTwitter(placePageData)
  }
  
  func didPressVk() {
    MWMPlacePageManagerHelper.openVk(placePageData)
  }
  
  func didPressLine() {
    MWMPlacePageManagerHelper.openLine(placePageData)
  }
  
  func didPressEmail() {
    MWMPlacePageManagerHelper.openEmail(placePageData)
  }
  
  func didCopy(_ content: String) {
    UIPasteboard.general.string = content
    let message = String(format: L("copied_to_clipboard"), content)
    UIImpactFeedbackGenerator(style: .medium).impactOccurred()
    presenter?.showToast(message)
  }

  func didPressOpenInApp(from sourceView: UIView) {
    let availableApps = OpenInApplication.availableApps
    guard !availableApps.isEmpty else {
      LOG(.warning, "Applications selection sheet should not be presented when the list of available applications is empty.")
      return
    }
    let openInAppActionSheet = UIAlertController.presentInAppActionSheet(from: sourceView, apps: availableApps) { [weak self] selectedApp in
      guard let self else { return }
      let link = selectedApp.linkWith(coordinates: self.placePageData.locationCoordinate, destinationName: self.placePageData.previewData.title)
      self.presenter?.openURL(link)
    }
    presenter?.showAlert(openInAppActionSheet)
  }
}

// MARK: - WikiDescriptionViewControllerDelegate

extension PlacePageInteractor: WikiDescriptionViewControllerDelegate {
  func didPressMore() {
    MWMPlacePageManagerHelper.showPlaceDescription(placePageData.wikiDescriptionHtml)
  }

  func didPressWikipedia() {
    MWMPlacePageManagerHelper.openWikipedia(placePageData)
  }
}

// MARK: - PlacePageButtonsViewControllerDelegate

extension PlacePageInteractor: PlacePageOSMContributionViewControllerDelegate {
  func didPressAddPlace() {
    MWMPlacePageManagerHelper.addPlace(placePageData.locationCoordinate)
  }

  func didPressEditPlace() {
    MWMPlacePageManagerHelper.editPlace()
  }

  func didPressUpdateMap() {
    startMapDownloading()
  }

  func didPressOSMInfo() {
    presenter?.openURL("https://welcome.openstreetmap.org")
  }
}

// MARK: - PlacePageEditBookmarkOrTrackViewControllerDelegate

extension PlacePageInteractor: PlacePageEditBookmarkOrTrackViewControllerDelegate {
  func didUpdate(color: UIColor, category: MWMMarkGroupID, for data: PlacePageEditData) {
    switch data {
    case .bookmark(let bookmarkData):
      let bookmarkColor = BookmarkColor.bookmarkColor(from: color) ?? bookmarkData.color
      MWMPlacePageManagerHelper.updateBookmark(placePageData,
                                               title: placePageData.previewData.title,
                                               color: bookmarkColor,
                                               category: category)
    case .track:
      MWMPlacePageManagerHelper.updateTrack(placePageData,
                                            title: placePageData.previewData.title,
                                            color: color,
                                            category: category)
    }
  }
  
  func didPressEdit(_ data: PlacePageEditData) {
    switch data {
    case .bookmark:
      MWMPlacePageManagerHelper.editBookmark(placePageData)
    case .track:
      MWMPlacePageManagerHelper.editTrack(placePageData)
    }
  }
}

// MARK: - ActionBarViewControllerDelegate

extension PlacePageInteractor: ActionBarViewControllerDelegate {
  func actionBar(_ actionBar: ActionBarViewController, didPressButton type: ActionBarButtonType) {
    switch type {
    case .booking:
      MWMPlacePageManagerHelper.book(placePageData)
    case .bookingSearch:
      MWMPlacePageManagerHelper.searchBookingHotels(placePageData)
    case .bookmark:
      if placePageData.bookmarkData != nil {
        MWMPlacePageManagerHelper.removeBookmark(placePageData)
      } else {
        MWMPlacePageManagerHelper.addBookmark(placePageData)
      }
    case .call:
      // since `.call` is a case in an obj-c enum, it can't have associated data, so there is no easy way to
      // pass the exact phone, and we have to ask the user here which one to use, if there are multiple ones
      let phones = placePageData.infoData?.phones ?? []
      let hasOnePhoneNumber = phones.count == 1
      if hasOnePhoneNumber {
        MWMPlacePageManagerHelper.call(phones[0])
      } else if (phones.count > 1) {
        showPhoneNumberPicker(phones, handler: MWMPlacePageManagerHelper.call)
      }
    case .download:
      startMapDownloading()
    case .opentable:
      fatalError("Opentable is not supported and will be deleted")
    case .routeAddStop, .routeReplaceStop:
      MWMPlacePageManagerHelper.routeAddStop(placePageData)
    case .routeFrom:
      MWMPlacePageManagerHelper.route(from: placePageData)
    case .routeRemoveStop:
      MWMPlacePageManagerHelper.routeRemoveStop(placePageData)
    case .routeTo:
      MWMPlacePageManagerHelper.route(to: placePageData)
    case .avoidToll:
      MWMPlacePageManagerHelper.avoidToll()
    case .avoidDirty:
      MWMPlacePageManagerHelper.avoidDirty()
    case .avoidFerry:
      MWMPlacePageManagerHelper.avoidFerry()
    case .more:
      fatalError("More button should've been handled in ActionBarViewContoller")
    case .track:
      guard placePageData.trackData != nil else { return }
      showTrackDeletionConfirmationDialog()
    case .saveTrackRecording:
      trackRecordingManager.stopAndSave() { [weak self] result in
        switch result {
        case .success:
          break
        case .trackIsEmpty:
          self?.presenter?.close()
        }
      }
    case .deleteTrackRecording:
      showTrackRecordingDiscardingConfirmationDialog()
    @unknown default:
      fatalError()
    }
  }

  private func startMapDownloading() {
    guard let mapNodeAttributes = placePageData.mapNodeAttributes else {
      fatalError("Download button can't be displayed if mapNodeAttributes is empty")
    }
    switch mapNodeAttributes.nodeStatus {
    case .downloading, .inQueue, .applying:
      Storage.shared().cancelDownloadNode(mapNodeAttributes.countryId)
    case .notDownloaded, .partly, .onDiskOutOfDate, .error:
      Storage.shared().downloadNode(mapNodeAttributes.countryId)
    case .undefined, .onDisk:
      fatalError("Download button shouldn't be displayed when node is in these states")
    @unknown default:
      fatalError()
    }
  }

  private func showTrackDeletionConfirmationDialog() {
    let alert = UIAlertController(title: nil,
                                  message: L("placepage_delete_track_confirmation_alert_message"),
                                  preferredStyle: .actionSheet)
    let deleteAction = UIAlertAction(title: L("delete"), style: .destructive) { [weak self] _ in
      guard let self = self else { return }
      guard self.placePageData.trackData != nil else {
        fatalError("The track data should not be nil during the track deletion")
      }
      MWMPlacePageManagerHelper.removeTrack(self.placePageData)
      self.presenter?.close()
    }
    let cancelAction = UIAlertAction(title: L("cancel"), style: .cancel)
    alert.addAction(deleteAction)
    alert.addAction(cancelAction)
    presenter?.showAlert(alert)
  }

  private func showTrackRecordingDiscardingConfirmationDialog() {
    let alert = UIAlertController(title: nil,
                                  message: L("placepage_delete_track_confirmation_alert_message"),
                                  preferredStyle: .actionSheet)
    let discardAction = UIAlertAction(title: L("delete"), style: .destructive) { [weak self] _ in
      guard let self = self else { return }
      self.trackRecordingManager.discard()
      self.presenter?.close()
    }
    let continueAction = UIAlertAction(title: L("continue_button"), style: .default)
    alert.addAction(discardAction)
    alert.addAction(continueAction)
    presenter?.showAlert(alert)
  }

  private func showPhoneNumberPicker(_ phones: [PlacePagePhone], handler: @escaping (PlacePagePhone) -> Void) {
    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    phones.forEach({phone in
      alert.addAction(UIAlertAction(title: phone.phone, style: .default, handler: { _ in
        handler(phone)
      }))
    })
    alert.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
    presenter?.showAlert(alert)
  }
}

// MARK: - ElevationProfileViewControllerDelegate

extension PlacePageInteractor: ElevationProfileViewControllerDelegate {
  func openDifficultyPopup() {
    MWMPlacePageManagerHelper.openElevationDifficultPopup(placePageData)
  }

  func updateMapPoint(_ point: CLLocationCoordinate2D, distance: Double) {
    guard let trackData = placePageData.trackData, trackData.elevationProfileData?.isTrackRecording == false else { return }
    bookmarksManager.setElevationActivePoint(point, distance: distance, trackId: trackData.trackId)
    placePageData.trackData?.updateActivePointDistance(distance)
  }
}

// MARK: - PlacePageHeaderViewController

extension PlacePageInteractor: PlacePageHeaderViewControllerDelegate {
  func previewDidPressClose() {
    presenter?.close()
  }

  func previewDidPressExpand() {
    presenter?.showNextStop()
  }

  func previewDidPressShare(from sourceView: UIView) {
    presenter?.showShareSheet(for: placePageData, from: sourceView)
  }

  func previewDidPressExportTrack(_ type: KmlFileType, from sourceView: UIView) {
    guard let trackId = placePageData.trackData?.trackId else {
      fatalError("Track data should not be nil during the track export")
    }
    bookmarksManager.shareTrack(trackId, fileType: type) { [weak self] status, url in
      guard let self else { return }
      switch status {
      case .success:
        guard let url else { fatalError("Invalid sharing url") }
        let shareViewController = ActivityViewController.share(for: url, message: self.placePageData.previewData.title!) { _,_,_,_ in
          self.bookmarksManager.finishSharing()
        }
        self.presenter?.showActivity(shareViewController, from: sourceView)
      case .emptyCategory:
        self.presenter?.showInfoAlert(title: L("bookmarks_error_title_share_empty"),
                                      message: L("bookmarks_error_message_share_empty"))
      case .archiveError, .fileError:
        self.presenter?.showInfoAlert(title: L("dialog_routing_system_error"),
                                      message: L("bookmarks_error_message_share_general"))
      }
    }
  }

  func previewDidCopy(_ content: String) {
    didCopy(content)
  }

  func previewDidFinishEditingTitle(_ newTitle: String) {
    switch placePageData.objectType {
    case .bookmark:
      guard let bookmarkData = placePageData.bookmarkData else {
        fatalError("bookmarkData can't be nil")
      }
      MWMPlacePageManagerHelper.updateBookmark(placePageData,
                                               title: newTitle,
                                               color: bookmarkData.color,
                                               category: bookmarkData.bookmarkGroupId)
    case .track:
      guard let trackData = placePageData.trackData else {
        fatalError("trackData can't be nil")
      }
      MWMPlacePageManagerHelper.updateTrack(placePageData,
                                            title: newTitle,
                                            color: trackData.color,
                                            category: trackData.groupId)
    default:
      fatalError("Editing title is only supported for bookmarks and tracks")
    }
  }
}

// MARK: - BookmarksObserver
extension PlacePageInteractor: BookmarksObserver {
  func onBookmarksLoadFinished() {
    updatePlacePageIfNeeded()
  }

  func onBookmarksCategoryDeleted(_ groupId: MWMMarkGroupID) {
    guard let bookmarkGroupId = placePageData.bookmarkData?.bookmarkGroupId else { return }
    if bookmarkGroupId == groupId {
      presenter?.close()
    }
  }
}
