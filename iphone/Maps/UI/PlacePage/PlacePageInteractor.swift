protocol PlacePageInteractorProtocol: AnyObject {
  func viewWillAppear()
  func updateTopBound(_ bound: CGFloat, duration: TimeInterval)
}

class PlacePageInteractor: NSObject {
  var presenter: PlacePagePresenterProtocol?
  weak var viewController: UIViewController?
  weak var mapViewController: MapViewController?
  private let bookmarksManager = BookmarksManager.shared()
  private var placePageData: PlacePageData
  private var viewWillAppearIsCalledForTheFirstTime = false

  init(viewController: UIViewController, data: PlacePageData, mapViewController: MapViewController) {
    self.placePageData = data
    self.viewController = viewController
    self.mapViewController = mapViewController
    super.init()
    addToBookmarksManagerObserverList()
  }

  deinit {
    removeFromBookmarksManagerObserverList()
  }

  private func updatePlacePageIfNeeded() {
    func updatePlacePage() {
      FrameworkHelper.updatePlacePageData()
      placePageData.updateBookmarkStatus()
    }

    switch placePageData.objectType {
    case .POI, .trackRecording:
      break
    case .bookmark:
      guard bookmarksManager.hasBookmark(placePageData.bookmarkData!.bookmarkId) else {
        presenter?.closeAnimated()
        return
      }
      updatePlacePage()
    case .track:
      guard bookmarksManager.hasTrack(placePageData.trackData!.trackId) else {
        presenter?.closeAnimated()
        return
      }
      updatePlacePage()
    @unknown default:
      fatalError("Unknown object type")
    }
  }

  private func addToBookmarksManagerObserverList() {
    bookmarksManager.add(self)
  }

  private func removeFromBookmarksManagerObserverList() {
    bookmarksManager.remove(self)
  }
}

extension PlacePageInteractor: PlacePageInteractorProtocol {
  func viewWillAppear() {
    // Skip data reloading on the first appearance, to avoid unnecessary updates.
    guard viewWillAppearIsCalledForTheFirstTime else {
      viewWillAppearIsCalledForTheFirstTime = true
      return
    }
    updatePlacePageIfNeeded()
  }

  func updateTopBound(_ bound: CGFloat, duration: TimeInterval) {
    mapViewController?.setPlacePageTopBound(bound, duration: duration)
  }
}

// MARK: - PlacePageInfoViewControllerDelegate

extension PlacePageInteractor: PlacePageInfoViewControllerDelegate {
  var shouldShowOpenInApp: Bool {
    !OpenInApplication.availableApps.isEmpty
  }

  func didPressCall() {
    MWMPlacePageManagerHelper.call(placePageData)
  }

  func didPressWebsite() {
    MWMPlacePageManagerHelper.openWebsite(placePageData)
  }

  func didPressWebsiteMenu() {
    MWMPlacePageManagerHelper.openWebsiteMenu(placePageData)
  }

  func didPressKayak() {
    let kUDDidShowKayakInformationDialog = "kUDDidShowKayakInformationDialog"
    
    if UserDefaults.standard.bool(forKey: kUDDidShowKayakInformationDialog) {
      MWMPlacePageManagerHelper.openKayak(placePageData)
    } else { 
      let alert = UIAlertController(title: nil, message: L("dialog_kayak_disclaimer"), preferredStyle: .alert)
      alert.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
      alert.addAction(UIAlertAction(title: L("dialog_kayak_button"), style: .default, handler: { _ in
        UserDefaults.standard.set(true, forKey: kUDDidShowKayakInformationDialog)
        MWMPlacePageManagerHelper.openKayak(self.placePageData)
      }))
      presenter?.showAlert(alert)
    }
  }

  func didPressWikipedia() {
    MWMPlacePageManagerHelper.openWikipedia(placePageData)
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
    Toast.toast(withText: message).show(withAlignment: .bottom)
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
      self.mapViewController?.openUrl(link, externally: true)
    }
    presenter?.showAlert(openInAppActionSheet)
  }
}

// MARK: - WikiDescriptionViewControllerDelegate

extension PlacePageInteractor: WikiDescriptionViewControllerDelegate {
  func didPressMore() {
    MWMPlacePageManagerHelper.showPlaceDescription(placePageData.wikiDescriptionHtml)
  }
}

// MARK: - PlacePageButtonsViewControllerDelegate

extension PlacePageInteractor: PlacePageButtonsViewControllerDelegate {
  func didPressHotels() {
    MWMPlacePageManagerHelper.openDescriptionUrl(placePageData)
  }

  func didPressAddPlace() {
    MWMPlacePageManagerHelper.addPlace(placePageData.locationCoordinate)
  }

  func didPressEditPlace() {
    MWMPlacePageManagerHelper.editPlace()
  }

  func didPressAddBusiness() {
    MWMPlacePageManagerHelper.addBusiness()
  }
}

// MARK: - PlacePageEditBookmarkOrTrackViewControllerDelegate

extension PlacePageInteractor: PlacePageEditBookmarkOrTrackViewControllerDelegate {
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
      MWMPlacePageManagerHelper.call(placePageData)
    case .download:
      guard let mapNodeAttributes = placePageData.mapNodeAttributes else {
        fatalError("Download button can't be displayed if mapNodeAttributes is empty")
      }
      switch mapNodeAttributes.nodeStatus {
      case .downloading, .inQueue, .applying:
        Storage.shared().cancelDownloadNode(mapNodeAttributes.countryId)
      case .notDownloaded, .partly, .error:
        Storage.shared().downloadNode(mapNodeAttributes.countryId)
      case .undefined, .onDiskOutOfDate, .onDisk:
        fatalError("Download button shouldn't be displayed when node is in these states")
      @unknown default:
        fatalError()
      }
    case .opentable:
      fatalError("Opentable is not supported and will be deleted")
    case .routeAddStop:
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
      // TODO: (KK) This is temporary solution. Remove the dialog and use the MWMPlacePageManagerHelper.removeTrack
      // directly here when the track recovery mechanism will be implemented.
      showTrackDeletionConfirmationDialog()
    case .saveTrackRecording:
      // TODO: (KK) pass name typed by user
      TrackRecordingManager.shared.processAction(.stopAndSave(name: "")) { [weak self] result in
        switch result {
        case .success:
          break
        case .error:
          self?.presenter?.closeAnimated()
        }
      }
    @unknown default:
      fatalError()
    }
  }

  private func showTrackDeletionConfirmationDialog() {
    let alert = UIAlertController(title: nil, message: L("placepage_delete_track_confirmation_alert_message"), preferredStyle: .actionSheet)
    let deleteAction = UIAlertAction(title: L("delete"), style: .destructive) { [weak self] _ in
      guard let self = self else { return }
      guard self.placePageData.trackData != nil else {
        fatalError("The track data should not be nil during the track deletion")
      }
      MWMPlacePageManagerHelper.removeTrack(self.placePageData)
      self.presenter?.closeAnimated()
    }
    let cancelAction = UIAlertAction(title: L("cancel"), style: .cancel)
    alert.addAction(deleteAction)
    alert.addAction(cancelAction)
    guard let viewController else { return }
    iPadSpecific {
      alert.popoverPresentationController?.sourceView = viewController.view
      alert.popoverPresentationController?.sourceRect = viewController.view.frame
    }
    viewController.present(alert, animated: true)
  }
}

// MARK: - ElevationProfileViewControllerDelegate

extension PlacePageInteractor: ElevationProfileViewControllerDelegate {
  func openDifficultyPopup() {
    MWMPlacePageManagerHelper.openElevationDifficultPopup(placePageData)
  }

  func updateMapPoint(_ point: CLLocationCoordinate2D, distance: Double) {
    guard let trackData = placePageData.trackData, trackData.elevationProfileData?.isTrackRecording == false else { return }
    BookmarksManager.shared().setElevationActivePoint(point, distance: distance, trackId: trackData.trackId)
  }
}

// MARK: - PlacePageHeaderViewController

extension PlacePageInteractor: PlacePageHeaderViewControllerDelegate {
  func previewDidPressClose() {
    presenter?.closeAnimated()
  }

  func previewDidPressExpand() {
    presenter?.showNextStop()
  }

  func previewDidPressShare(from sourceView: UIView) {
    guard let mapViewController else { return }
    switch placePageData.objectType {
    case .POI, .bookmark:
      let shareViewController = ActivityViewController.share(forPlacePage: placePageData)
      shareViewController.present(inParentViewController: mapViewController, anchorView: sourceView)
    case .track:
      presenter?.showShareTrackMenu()
    case .trackRecording:
      let currentLocation = LocationManager.lastLocation()?.coordinate ?? placePageData.locationCoordinate
      let shareMyPositionViewController = ActivityViewController.share(forMyPosition: currentLocation)
      shareMyPositionViewController.present(inParentViewController: mapViewController, anchorView: sourceView)
    default:
      fatalError()
    }
  }

  func previewDidPressExportTrack(_ type: KmlFileType, from sourceView: UIView) {
    guard let trackId = placePageData.trackData?.trackId else {
      fatalError("Track data should not be nil during the track export")
    }
    bookmarksManager.shareTrack(trackId, fileType: type) { [weak self] status, url in
      guard let self, let mapViewController else { return }
      switch status {
      case .success:
        guard let url else { fatalError("Invalid sharing url") }
        let shareViewController = ActivityViewController.share(for: url, message: self.placePageData.previewData.title!) { _,_,_,_ in
          self.bookmarksManager.finishSharing()
        }
        shareViewController.present(inParentViewController: mapViewController, anchorView: sourceView)
      case .emptyCategory:
        self.showAlert(withTitle: L("bookmarks_error_title_share_empty"),
                        message: L("bookmarks_error_message_share_empty"))
      case .archiveError, .fileError:
        self.showAlert(withTitle: L("dialog_routing_system_error"),
                        message: L("bookmarks_error_message_share_general"))
      }
    }
  }

  private func showAlert(withTitle title: String, message: String) {
    MWMAlertViewController.activeAlert().presentInfoAlert(title, text: message)
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
      presenter?.closeAnimated()
    }
  }
}

