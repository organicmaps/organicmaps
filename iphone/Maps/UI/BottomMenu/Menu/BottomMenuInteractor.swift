protocol BottomMenuInteractorProtocol: AnyObject {
  func close()
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
  weak var presenter: BottomMenuPresenterProtocol?
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
  func close() {
    guard let controlsManager = controlsManager else {
      fatalError()
    }
    controlsManager.menuState = controlsManager.menuRestoreState
  }

  func addPlace() {
    delegate?.addPlace()
  }

  func donate() {
    close()
    guard var url = Settings.donateUrl() else { return }
    if url == "https://organicmaps.app/donate/" {
      url = L("translated_om_site_url") + "donate/"
    }
    viewController?.openUrl(url, externally: true)
  }

  func downloadMaps() {
    close()
    delegate?.actionDownloadMaps(.downloaded)
  }

  func openSettings() {
    close()
    mapViewController?.openSettings()
  }

  func startDownloadingMapForCountry(_ countryId: String) {
    let mapNodeAttributes = mapsStorage.attributes(forCountry: countryId)
    switch mapNodeAttributes.nodeStatus {
    case .downloading, .inQueue, .applying:
      break
    case .notDownloaded, .partly, .onDiskOutOfDate, .error:
      mapsStorage.downloadNode(countryId)
    case .undefined, .onDisk:
      fatalError("Download button shouldn't be displayed when node is in these states")
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
