protocol BottomMenuInteractorProtocol: AnyObject {
  func close()
  func returnToCaller()
  func addPlace()
  func downloadMaps()
  func startDownloadingMapForCountry(_ countryId: String)
  func donate()
  func openSettings()
  func shareLocation(cell: BottomMenuItemCell)
  func toggleTrackRecording()
}

@objc protocol BottomMenuDelegate {
  func actionDownloadMaps(_ mode: MWMMapDownloaderMode)
  func addPlace()
  func didFinishAddingPlace()
}

class BottomMenuInteractor {
  private weak var viewController: UIViewController?
  private weak var mapViewController: MapViewController?
  private weak var delegate: BottomMenuDelegate?
  private weak var controlsManager: MWMMapViewControlsManager?

  private let mapsStorage = Storage.shared()
  private let trackRecorder: TrackRecordingManager = .shared

  init(viewController: UIViewController,
       mapViewController: MapViewController,
       controlsManager: MWMMapViewControlsManager,
       delegate: BottomMenuDelegate) {
    self.viewController = viewController
    self.mapViewController = mapViewController
    self.delegate = delegate
    self.controlsManager = controlsManager
  }
}

extension BottomMenuInteractor: BottomMenuInteractorProtocol {
  func returnToCaller() {
    close()
    mapViewController?.goBack()
  }

  func close() {
    guard let controlsManager = controlsManager else {
      fatalError()
    }
    controlsManager.menuState = controlsManager.closedMenuState
  }

  func addPlace() {
    delegate?.addPlace()
  }

  func donate() {
    close()
    if let url = Settings.donateUrl(), viewController?.openUrl(url, externally: true) == true {
      Settings.didShowDonationPage()
    }
  }

  func downloadMaps() {
    close()
    delegate?.actionDownloadMaps(.downloaded)
  }

  func openSettings() {
    mapViewController?.openSettings()
  }

  func startDownloadingMapForCountry(_ countryId: String) {
    let mapNodeAttributes = mapsStorage.attributes(forCountry: countryId)
    // Terrain does not affect map editing eligibility.
    switch mapNodeAttributes.mapStatus {
    case .onDisk:
      // The map may have finished updating while the confirmation was open.
      if MWMNavigationDashboardManager.shared().state == .closed,
         mapsStorage.countryForViewportCenter() == countryId {
        addPlace()
      } else {
        close()
      }
      return
    case .downloading, .inQueue, .applying:
      break
    case .onDiskOutOfDate:
      mapsStorage.updateNode(countryId)
    case .error:
      mapsStorage.retryDownloadNode(withAlerts: countryId)
    case .notDownloaded, .partly:
      mapsStorage.downloadNode(countryId)
    case .undefined:
      fatalError("Download button shouldn't be displayed for an undefined map status")
    @unknown default:
      fatalError()
    }
    downloadMaps()
  }

  func shareLocation(cell: BottomMenuItemCell) {
    guard let coordinates = LocationManager.lastLocation()?.coordinate else {
      viewController?.present(UIAlertController.unknownCurrentPosition(), animated: true, completion: nil)
      return
    }
    guard let viewController = viewController else { return }
    let vc = ActivityViewController.share(forMyPosition: coordinates)
    vc.present(inParentViewController: viewController, anchorView: cell.anchorView)
  }

  func toggleTrackRecording() {
    close()
    let mapViewController = MapViewController.shared()!
    switch trackRecorder.recordingState {
    case .active:
      mapViewController.showTrackRecordingPlacePage()
    case .inactive:
      trackRecorder.start { result in
        switch result {
        case .success:
          mapViewController.showTrackRecordingPlacePage()
        case .failure:
          break
        }
      }
    }
  }
}
